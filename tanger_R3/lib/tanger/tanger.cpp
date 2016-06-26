/* Copyright (C) 2007-20010  Torvald Riegel
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, version 2
 * of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
/**
 * The Tanger pass.
 *
 * To get an idea about how it works, start at processModule() and follow
 * the control-flow.
 */
#define DEBUG_TYPE "tanger"

#include "llvm/Pass.h"
#include "llvm/Module.h"
#include "llvm/Function.h"
#include "llvm/BasicBlock.h"
#include "llvm/Instructions.h"
#include "llvm/Constants.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CFG.h"
#include "llvm/Support/CallSite.h"
#include "llvm/DerivedTypes.h"
#include "llvm/IntrinsicInst.h"
#include "llvm/InlineAsm.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Transforms/Utils/Local.h"
#include "llvm/Support/CommandLine.h"

#include <fstream>
#include <sstream>
#include "tanger.h"
#include "support.h"

using namespace llvm;
using namespace tanger;

namespace {

// Definition of statistics
STATISTIC(stat_begin, "Number of begin() calls");
STATISTIC(stat_potentialIndirectCallTargets, "Number of potential targets of indirect calls");
STATISTIC(stat_validIndirectCallTargets, "Number of tm-safe targets of indirect calls");
STATISTIC(stat_instrsDemToStack, "Number of instructions in regs that are demoted to stack slots");

cl::opt<bool> clWholeProgram("tanger-whole-program", cl::init(true), cl::desc(
        "True iff the module contains all txns in the program"));
cl::opt<bool> clExitOnError("tanger-exit-on-error", cl::init(true), cl::desc(
        "Exit with non-zero error code on errors"), cl::Hidden);
cl::opt<bool> clIndirectAuto("tanger-indirect-auto", cl::init(true), cl::desc(
        "Automatically detect indirect call targets"));
cl::opt<bool> clResolveNontxnalDirectCalls("tanger-resolve-nontxnal", cl::init(
        true), cl::desc("Resolve direct calls to functions with no txnal version"));
cl::opt<std::string>
        clMultipleTMSupport(
                "tm-support-file",
                cl::value_desc("filename"),
                cl::desc(
                        "Selects+describes the TM implementations for which instrumentations are to be generated."));

} //namespace

/** The prefix for transactional versions of functions. */
static const char* tanger_txnal_func_prefix = "tanger_txnal_";
static const char* tanger_txnal_func_missing_prefix = "tanger_txnal_missing_";

char TangerTransform::ID = 0;

namespace {
RegisterPass<TangerTransform> PassRegistration1("tanger",
        "Transform transaction statements");
}


TangerTransform::TangerTransform(char& id) :
    ModulePass(id), noChangeTransformer(~(uint32_t) 0, ~(uint32_t) 0), M(0)
{
}


void TangerTransform::loadTMConfig(const char *fileName)
{
    std::ifstream in(fileName);
    if (!in.good()) {
        printWarning() << "Could not load file '" << fileName
                << "'! Continuing as if it's empty.\n";
        return;
    }

    while (in) {
        std::string symbol;
        in >> symbol;
        size_t p = symbol.find(',');
        if (p != std::string::npos) {
            uint32_t id;
            std::istringstream(std::string(symbol, p + 1)) >> id;
            supportedTMs.push_back(std::make_pair(symbol.substr(0, p), id));
            DEBUG(errs() << "supported TM suffix=" << symbol.substr(0, p)
                    << " id=" << id << "\n");
        }
        else if (!symbol.empty()) {
            printError() << "cannot parse TM config file " << fileName <<
                    ": " << symbol << "\n";
            if (clExitOnError) exit(1);
        }
    }
}


void TangerTransform::getAnalysisUsage(AnalysisUsage &AU) const
{
    AU.addRequired<TargetData> ();
}


bool TangerTransform::runOnModule(Module& M)
{
    this->M = &M;
    processModuleInit();
    processModule();

    // release memory
    levelOneFunctions.clear();
    txnalFunctions.clear();
    potentialIndirectCallTargets.clear();
    for (FuncInfoMap::iterator i = funcInfos.begin(), ie = funcInfos.end();
            i != ie; i++)
        delete i->second;
    funcInfos.clear();

    return true;
}


void TangerTransform::processModuleInit()
{
    DEBUG(errs() << "PROCESSING MODULE " << M->getModuleIdentifier() << "\n");

    // initialize interface first
    tif.init(M, &getAnalysis<TargetData> ());

    // load supported TM implementations
    if (!clMultipleTMSupport.empty()) {
        loadTMConfig(clMultipleTMSupport.c_str());
    }
    // If we have no information, just use the default instrumented version.
    // XXX: derive ID from something, 1 works just with the current ABI specs
    // (a_runInstrumentedCode is 1, so the effective instrumentation version
    // is 0).
    if (supportedTMs.empty()) supportedTMs.push_back(std::make_pair("", (uint32_t)1));

    // Create default transformers.
    // TODO set a correct ABI ID.
    // ABIID = internalID + 1
    for (unsigned i = 0; i < supportedTMs.size(); i++)
        transformers.push_back(new DefaultTransformer(&tif, this, supportedTMs[i].second, i,
                supportedTMs[i].first));

    // init common transformations
    if (!transform.init(&tif)) {
        errs() << "Error in initialization, stopping processing\n";
        if (clExitOnError) exit(1);
    }
}


/**
 * Transforms a module.
 *
 * It starts by (1) looking for wrapper declarations, annotations, and
 * potential targets of indirect calls. This information is put into FuncInfo
 * objects associated with the original functions.
 * (2), we first analyze functions that call tanger_begin() (ie, functions
 * that start new top-level txns). If the analysis finds functions that are
 * called from txnal context, these functions are put into a set of functions
 * that are called from txnal context and thus need txnal versions.
 * (3), we use this set to analyze all these functions and to find all
 * other functions that need txnal versions.
 * (4), we first create txnal versions by cloning the original functions and
 * then transforming them. Once that is done, we transform only the in-txn
 * parts of the original functions that start top-level txns.
 * (5), we check some simple constraints such as whether init and shutdown
 * functions are called if there are any txns.
 * (6), we add additional initialization calls and clean up.
 */
void TangerTransform::processModule()
{
    bool error = false;

    // (1) find wrapper declarations and pure declarations
    error = error || findTxnalWrapperDeclarations();
    error = error || addDefaultTxnalWrapperDeclarations();
    // pre-populate function information using annotations
    // and potential indirect call targets
    for (Module::iterator i = M->begin(), ie = M->end(); i != ie; ++i) {
        Function* f = i;
        FuncInfo* fi = 0;
        // parse annotations
        // TODO parse annotations or the special attributes defined in the
        //   TM language specs
        // if the function does not access memory and does not write anything,
        // then no instrumentation is necessary, and we mark it as pure
        if (f->hasFnAttr(Attribute::ReadNone)) {
            if (fi == 0) fi = getOrCreateFuncInfo(f);
            if (fi->type != FuncInfo::Pure) {
                fi->setPure(f);
                DEBUG(errs() << "Function " << f->getName() << " is pure\n");
            }
        }
        // automatic indirect calls?
        if (clIndirectAuto) {
            for (Value::use_iterator j = f->use_begin(), je = f->use_end();
                    j != je; ++j) {
                // Skip all alias definitions and annotations.
                User *u = *j;
                if (isa<GlobalAlias> (u)) continue;
                if (Constant* x = dyn_cast<ConstantExpr>(u)) {
                    Value::use_iterator jj, jje;
                    // Check whether all users are annotations.
                    for (jj = x->use_begin(), jje = x->use_end(); jj != jje; ++jj) {
                        User *uu = *jj;
                        if (isa<ConstantStruct>(uu)) {
                            if (uu->getNumUses() == 1) uu = *uu->use_begin();
                            if (!isa<ConstantArray>(uu)) break;
                            if (uu->getNumUses() == 1) uu = *uu->use_begin();
                            if (!isa<GlobalVariable>(uu) || uu->getNameStr() != "llvm.global.annotations") break;
                        }
                        else if (isa<ConstantArray>(uu)) {
                            if (uu->getNumUses() == 1) uu = *uu->use_begin();
                            if (!isa<GlobalVariable>(uu) || uu->getNameStr() != "llvm.used") break;
                        }
                        else break;
                    }
                    // If only used in annotations, skip it.
                    if (jj == jje) continue;
                }
                CallInst *ci = dyn_cast<CallInst> (u);
                if (!ci || ci->getCalledFunction() != f) {
                    // Function has a user that is not a call.
                    if (fi == 0) fi = getOrCreateFuncInfo(f);
                    if (fi->type == FuncInfo::Unknown) {
                        // There is no info about the function yet. Assume that
                        // it's a potential call target. If we already had a type, this function
                        // might only be used by one of our wrapper declarations.
                        fi->type = FuncInfo::PotentiallyCallable;
                    }
                    if (fi->type != FuncInfo::TxnalVersion) {
                        // It is not a txnal wrapper or version, so can be
                        // potentially called from the original code.
                        potentialIndirectCallTargets.insert(f);
                        DEBUG(errs() << "Potential indirect call target: " << f->getName() << "\n");
                        stat_potentialIndirectCallTargets++;
                    }
                    break;
                }
            }
        }
    }

    // (2) Find all users of calls to tm_begin(). They have to be analyzed first.
    error = error || checkFunctionsWithTransactions();

    // (3) Find and analyze all functions that need txnal versions.
    if (clWholeProgram) {
        // Create txnal versions on demand. Add functions marked as callable.
        // Skip functions that have a txnal version because these already have wrappers.
        for (FuncInfoMap::iterator i = funcInfos.begin(), ie = funcInfos.end();
                i != ie; ++i) {
            if ((i->second->type == FuncInfo::Callable || i->second->type
                    == FuncInfo::PotentiallyCallable)
                    && !i->second->txnalVersionProcessed) {
                txnalFunctions.insert(i->first);
            }
        }
        while (!txnalFunctions.empty()) {
            Function *f = *txnalFunctions.begin();
            FuncInfo *fi = funcInfos[f];
            txnalFunctions.erase(txnalFunctions.begin());
            assert(!fi->txnalVersionProcessed);
            assert(fi->type != FuncInfo::Pure && fi->type != FuncInfo::TxnalVersion);

            // check function if necessary
            if (!fi->hasBeenChecked) {
                // analyze and check
                if (f->isDeclaration()) {
                    // This is the clWholeProgram==true path: we need to have all sources.
                    if (fi->type == FuncInfo::Callable) printWarning(*f)
                            << " function has no body. assuming that it cannot be transformed\n";
                    else
                    DEBUG(errs() << "Function " << f->getName() << " has no body. assuming that it cannot be transformed\n");
                    fi->checkResult = FuncInfo::NothingValid;
                    fi->hasBeenChecked = true;
                }
                else analyzeAndCheckFunction(*f, fi, &txnalFunctions);
            }
            if (fi->checkResult != FuncInfo::CompletelyValid) {
                // cannot create a txnal version
                if (fi->type == FuncInfo::PotentiallyCallable)
                DEBUG(errs() << "Function " << f->getName()
                        << " is a potential indirect call target (or callee of one), but has no txnal version\n");
                else if (fi->type == FuncInfo::Callable) {
                    if (clResolveNontxnalDirectCalls)
                        printWarning(*f)
                            << " function is callable from txnal context but cannot be transformed\n";
                    else {
                        printError(*f)
                                << " function is callable from txnal context but cannot be transformed\n";
                        error = true;
                        // We create function declarations for the txnal
                        // versions anyway because we want to propagate this
                        // error to the linker (there will be a call to a
                        // transactional function, but it does not have a
                        // function body).
                        for (unsigned i = 0; i < supportedTMs.size(); ++i) {
                            Function* tf = getTxnalFunction(f, supportedTMs[i].first);
                            FuncInfo* tfi = getOrCreateFuncInfo(tf);
                            tfi->setTxnalVersion(tf, i, f, fi);
                        }
                    }
                }
            }
            else {
                // create txnal version
                if (fi->type == FuncInfo::PotentiallyCallable) {
                    // set to potentially callable, don't set to to-be-transformed yet
                    setCallable(f, fi, &txnalFunctions, true);
                }
                else {
                    // set to Callable and transform
                    setCallable(f, fi, &txnalFunctions, false);
                }
            }
        }
    }
    else {
        // library mode: try to transform all functions that are callable or unknown.
        // Skip STM functions, functions with no body.
        errs() << "library transformation mode not yet implemented\n";
        exit(1);
    }
    // Check if all functions in call graph of potential indirect call targets
    // can be transformed
    for (FunctionSet::iterator i = potentialIndirectCallTargets.begin(),
            ie = potentialIndirectCallTargets.end(); i != ie; ++i) {
        // build a set of callees
        //DEBUG(errs() << "checking indirectCallTarget " << (*i)->getName() <<"\n");
        FunctionSet cg;
        FunctionSet todo;
        todo.insert(*i);
        bool result = true;
        while (!todo.empty()) {
            Function *f = *todo.begin();
            todo.erase(todo.begin());
            FuncInfo *fi = funcInfos[f];
            assert(fi->hasBeenChecked);
            assert(fi->type != FuncInfo::TxnalVersion);
            assert(fi->type != FuncInfo::Unknown);
            if (fi->type == FuncInfo::Pure) continue;
            // (Potentially) callable functions must be completely valid for transformation
            if (fi->checkResult != FuncInfo::CompletelyValid) {
                // (potentially) callable, but not all callees have txnal versions
                if (f != *i) {
                    printWarning(**i)
                            << " function is a potential indirect call target, but its callee "
                            << f->getName() << " has no txnal version\n";
                }
                else {
                    printWarning(**i)
                            << " function is a potential indirect call target but has no txnal version\n";
                }
                result = false;
                break;
            }
            if (cg.insert(f).second) {
                // new callee, check analysis results
                todo.insert(fi->levelZeroCallees.begin(),
                        fi->levelZeroCallees.end());
            }
        }
        if (result) {
            // all callees can be transformed, set CG funcs to callable
            //DEBUG(errs() << " setcallable " << (*i)->getName() <<"\n");
            setCallable(*i, funcInfos[*i], 0, false);
        }
    }

    // (4) Transform txnal versions and functions with txns.
    // Txnal versions exist for all functions that are marked callable,
    // are completely valid, and don't have a txnal version yet.
    // However, we first make sure that all txnal versions are declared.
    for (FuncInfoMap::iterator i = funcInfos.begin(), ie = funcInfos.end();
            i != ie; ++i) {
        bool tv = i->second->type == FuncInfo::Callable
                && i->second->checkResult == FuncInfo::CompletelyValid
                && !i->second->txnalVersionProcessed;
        if (!tv) continue;
        for (unsigned tm = 0; tm < supportedTMs.size(); ++tm) {
            if (i->second->getTxnalFunction(tm)) continue;
            Function* tf = getTxnalFunction(i->first, supportedTMs[tm].first);
            FuncInfo* tfi = getOrCreateFuncInfo(tf);
            tfi->setTxnalVersion(tf, tm, i->first, i->second);
            //DEBUG(errs() << i->first->getName() << " tm="<<tm<<"\n");
        }
    }
    for (FuncInfoMap::iterator i = funcInfos.begin(), ie = funcInfos.end();
            i != ie; ++i) {
        bool tv = i->second->type == FuncInfo::Callable
                && i->second->checkResult == FuncInfo::CompletelyValid
                && !i->second->txnalVersionProcessed;
        bool ht = levelOneFunctions.count(i->first);
        if (tv || ht) transformFunction(*i->first, i->second, ht, tv);
    }

    // any errors?
    if (error) errs() << "ERROR: transformation of functions did not succeed\n";

    // (5) do automatic STM setup
    transform.insertSTMSetup(lookupPassInfo(getPassID())->getPassArgument());

    // (6) insert various initialization hooks
    std::vector<CallInst*> initCalls;
    transform.getCalls(tif.getFunction(Interface::itm_initProcess), initCalls);
    // indirect call registration
    if (!potentialIndirectCallTargets.empty()) {
        DEBUG(errs() << "number of potential indirect-call targets: " << potentialIndirectCallTargets.size() << "\n");
        std::map<Function*, std::vector<Function*> > nontxnal2txnal;
        for (FunctionSet::iterator i = potentialIndirectCallTargets.begin(),
                ie = potentialIndirectCallTargets.end(); i != ie; ++i) {
            FuncInfo* fi = funcInfos[*i];
            // After being checked if possible, potential indirect call targets
            // must be of one of a few types.
            assert(fi->type == FuncInfo::Callable || fi->type == FuncInfo::PotentiallyCallable
                    || fi->type == FuncInfo::Pure);
            assert(fi->type != FuncInfo::Callable || fi->hasBeenChecked);
            // If a function is Callable, it must also be completely valid.
            if ((fi->type == FuncInfo::Callable && fi->checkResult == FuncInfo::CompletelyValid)
                    || fi->type == FuncInfo::Pure) {
                DEBUG(errs() << " valid indirect call target: " << (*i)->getName() << "\n");
                assert(fi->txnalVersionProcessed);
                nontxnal2txnal.insert(std::make_pair(*i,
                        fi->getTxnalFunctions()));
                stat_validIndirectCallTargets++;
            }
        }
        DEBUG(errs() << "number of valid indirect-call targets: " << nontxnal2txnal.size() << "\n");
        if (!nontxnal2txnal.empty()) {
            if (clWholeProgram) {
                // all potential indirect-call targets are Callable or Pure
                for (std::vector<CallInst*>::iterator i = initCalls.begin(),
                        ie = initCalls.end(); i != ie; i++)
                    transform.insertIndirectCallInit(*i, nontxnal2txnal);
            }
            else {
                errs() << "library transformation mode not yet implemented\n";
                exit(1);
            }
        }
    }

    // iterate over functions again and fix linkage of missing txnal functions
    if (clWholeProgram) {
        for (Module::iterator i = M->begin(), ie = M->end(); i != ie; ++i) {
            FuncInfo *fi = getOrCreateFuncInfo(&*i);
            if (fi->type == FuncInfo::Callable && fi->checkResult
                    != FuncInfo::CompletelyValid) {
                printWarning(*i)
                        << " function is Callable but it cannot have a txnal version\n";
                // Just setting linkage to external is not a very good
                // solution because there might be collision with existing
                // functions. A better way would be to redirect calls to missing
                // transactional functions to dummy functions, so that the
                // user still sees the problem in the linker, but we don't
                // have collisions with anything important.
                // As a work around, we additionally rename these functions.
                // If we do not have created a txnal clone yet (thus there is
                // also no caller, we skip this step as we can't propagate
                // an error to the linker anyway.
                for (size_t j = 0; j < supportedTMs.size(); ++j) {
                    Function * fnc = fi->getTxnalFunction(j);
                    if (fnc) {
                        std::string name = fnc->getName();
                        name.replace(0, strlen(tanger_txnal_func_prefix),
                                tanger_txnal_func_missing_prefix);
                        fnc->setName(name);
                        fnc->setLinkage(GlobalValue::ExternalLinkage);
                    }
                }
            }
        }
    }
    // return true to signal that Module has been modified
    DEBUG(errs() << "FINISHED MODULE " << M->getModuleIdentifier() << "\n");
    error |= hadFatalError();
    if (error) {
        errs() << "ERROR: transformation of module did not succeed\n";
        if (clExitOnError) exit(1);
    }
}


FuncInfo* TangerTransform::getOrCreateFuncInfo(Function* f)
{
    FuncInfoMap::iterator i = funcInfos.find(f);
    if (i == funcInfos.end()) return funcInfos.insert(i, std::make_pair(f,
            new FuncInfo(supportedTMs.size())))->second;
    else return i->second;
}

/**
 * Create or get the declaration of a wrapper function for a wrapped function.
 */
bool TangerTransform::createDeclaredTxnalWrapper(Function* original, std::string wrapperName)
{
    bool error = false;
    // Add call redirection mapping: Function is callable and txnal version is known
    FuncInfo *fi = getOrCreateFuncInfo(original);
    // create function declaration of wrapper if necessary
    Function *target = M->getFunction(wrapperName);
    if (!target) {
        // Create a declaration for the wrapper.
        target = dyn_cast<Function> (M->getOrInsertFunction(wrapperName,
                original->getFunctionType()));
    }
    else if (original->getFunctionType() != target->getFunctionType()) {
        // types of wrapper and original mismatch
        errs() << "ERROR: txnal wrapper function " << wrapperName
                << " exists with type " << *target->getFunctionType()
                << " but original function has type "
                << *original->getFunctionType()
                << ". Wrapper cannot be used.\n";
        error = true;
        // for wrappers, we only have the default txnal version
        target = getTxnalFunction(original, "");
    }
    target->copyAttributesFrom(original);
    // set wrapper information
    fi->setWrapped(target);
    // Mark target as txnal version
    getOrCreateFuncInfo(target)->setTxnalVersion(target, 0, original, fi, true);
    DEBUG(errs() << "Function " << original->getName() << " has wrapper " << target->getName() << "\n");
    return error;
}

/**
 * Looks for symbol aliases with tanger-wrapper prefixes.
 *
 * If an alias' target is actually used, the alias will either add an
 * annotation to the target or it will set the custom transactional version
 * of the target to the name of the alias.
 * Returns true on errors.
 */
bool TangerTransform::findTxnalWrapperDeclarations()
{
    bool error = false;
    for (Module::const_global_iterator I = M->global_begin(), E = M->global_end();
            I != E; ++I) {
       if (I->getNameStr() == "llvm.global.annotations") {
          ConstantArray *CA = dyn_cast<ConstantArray>(I->getInitializer());
          for (User::op_iterator i = CA->op_begin(), e = CA->op_end(); i != e; ++i)
              if (ConstantStruct *CS = dyn_cast<ConstantStruct>(*i)) {
                  Constant * CO = dyn_cast<Constant>(CS->getOperand(1)->getUnderlyingObject());
                  ConstantArray *CA2 = dyn_cast<ConstantArray>(CO->getOperand(0));
                  FuncInfo::Type type = FuncInfo::Unknown;
                  bool noinline = false;
                  bool alwaysinline = false;
                  int pos = 0;
                  std::string str = CA2->getAsString();

                  if (str.find(tif_txn_pure) == 0)
                      { type = FuncInfo::Pure; pos = strlen(tif_txn_pure); }
                  else if (str.find(tif_txn_callable) == 0)
                      { type = FuncInfo::Callable; pos = strlen(tif_txn_callable); }
                  else if (str.find(tif_txn_wrapper) == 0)
                      { type = FuncInfo::TxnalVersion; pos = strlen(tif_txn_wrapper); }
                  else continue;

                  if (str.find(tif_txn_noinline, pos) == 0)
                      { noinline = true; pos += strlen(tif_txn_noinline); }
                  else if (str.find(tif_txn_alwaysinline, pos) == 0)
                      { alwaysinline = true; pos += strlen(tif_txn_alwaysinline); }

                  Function * f = dyn_cast<Function>(CS->getOperand(0)->getUnderlyingObject());
                  if (f == 0)
                      continue;

                  if (type == FuncInfo::Pure) {
                      getOrCreateFuncInfo(f)->setPure(f);
                      DEBUG(errs() << "Function " << f->getName() << " is pure\n");
                  }
                  else if (type == FuncInfo::Callable) {
                      FuncInfo* fi = getOrCreateFuncInfo(f);
                      if (!f->isDeclaration()) {
                          // Function has a definition. Use PotentiallyCallable
                          // to first check that it is actually callable.
                          if (fi->type == FuncInfo::Unknown) {
                              // There is no info about the function yet. Assume that
                              // it's a potential call target. If we already had a type, this function
                              // might only be used by one of our wrapper declarations.
                              fi->type = FuncInfo::PotentiallyCallable;
                              potentialIndirectCallTargets.insert(f);
                              DEBUG(errs() << "Function " << f->getName() << " is callable (pending verification)\n");
                              stat_potentialIndirectCallTargets++;
                          }
                          else if (fi->type != FuncInfo::PotentiallyCallable)
                              DEBUG(errs() << "Function " << f->getName() << " is declared callable but is already of another type\n");
                      }
                      else {
                          // Function is a declaration only. Create an
                          // declaration-only txnal version for it.
                          setCallable(f, fi, 0, false);
                          fi->hasBeenChecked = true;
                          fi->txnalVersionProcessed = true;
                          for (unsigned tm = 0; tm < supportedTMs.size(); ++tm) {
                              if (fi->getTxnalFunction(tm)) continue;
                              Function* tf = getTxnalFunction(f, supportedTMs[tm].first);
                              FuncInfo* tfi = getOrCreateFuncInfo(tf);
                              tfi->setTxnalVersion(tf, tm, f, fi);
                              //DEBUG(errs() << i->first->getName() << " tm="<<tm<<"\n");
                          }
                          DEBUG(errs() << "Function " << f->getName() << " is callable\n");
                      }
                  }
                  else if (type == FuncInfo::TxnalVersion) {
                      // Extract
                      assert(str.at(pos) == '_');
                      pos++;
                      if (str.at(str.length() - 1) == 0) str.erase(str.length() - 1, 1);
                      error |= createDeclaredTxnalWrapper(f, str.substr(pos));
                  }

                  if (!noinline) f->removeFnAttr(Attribute::NoInline);
                  if (alwaysinline) f->addFnAttr(Attribute::AlwaysInline);
              }
       }
    }
    // find all aliases whose names start with the tanger_wrapper prefix
    for (Module::alias_iterator i = M->alias_begin(), ie = M->alias_end(),
            inext = i; i != ie; i = inext) {
        ++inext;
        bool pure = (i->getName().find(tif_txnalwrapperpure_prefix) == 0);
        bool isWrappedIntrinsic = (i->getAliasedGlobal()->getName().find(
                tif_txnalwrapperintrinsic_prefix) == 0);
        if (!pure && (i->getName().find(tif_txnalwrapper_prefix) != 0)
                && !isWrappedIntrinsic) continue;
        Function *original = const_cast<Function*> (dyn_cast<Function> (
                i->getAliasedGlobal()));
        if (!original) {
            errs() << "ERROR: alias " << i->getName()
                    << " has Tanger-wrapper prefix but does not reference a function:\n";
            i->getAliasedGlobal()->print(errs());
            error = true;
            continue;
        }
        if (isWrappedIntrinsic) {
            // get the name of the wrapped intrinsic
            std::string n(i->getAliasedGlobal()->getName(), strlen("tanger."));
            original = M->getFunction(n);
            // Ignore wrapper declaration if original is not used.
            // XXX We could check whether this name matches for some
            // (overloaded) intrinsic but this might be too time-consuming.
            if (!original) continue;
            if (!original->isIntrinsic()) {
                errs() << "Intrinsic wrapper used for " << n << ", which is not an intrinsic function.\n";
                error = true;
                continue;
            }
        }
        // The alias itself should have no users. The function implementing
        // the alias
        if (!i->use_empty()) printWarning() << " alias " << i->getName()
                << " has Tanger-wrapper prefix but is used directly\n";
        // Check that the target function is actually being used by someone
        // besides this alias. If not, remove both the alias and the function.
        if (!original->hasNUsesOrMore(2) && !isWrappedIntrinsic) {
            if (i->use_empty()) {
                i->eraseFromParent();
                original->eraseFromParent();
            }
            continue;
        }
        // We have a wrapper declaration.
        if (pure) {
            // Annotate target function to be pure
            getOrCreateFuncInfo(original)->setPure(original);
            DEBUG(errs() << "Function " << original->getName() << " is pure\n");
        }
        else {
            // Remove the marker prefix from the wrapper function name and
            // create the wrapper.
            std::string wrapperName = i->getNameStr().substr(strlen(tif_txnalwrapper_prefix));
            error |= createDeclaredTxnalWrapper(original, wrapperName);
        }

        // XXX This is a hack to work around an LLVM 2.6 issue when compiling QuakeTM.
        if (isWrappedIntrinsic) {
            // delete the aliasing function
            GlobalAlias * ferase = &*i;
            Function * val = dyn_cast<Function> (ferase->getAliasee());
            if (ferase /*&& ferase->use_empty()*/) ferase->eraseFromParent();
            assert(val->use_empty() && "Aliased function used.");
            val->eraseFromParent();
        }
    }
    return error;
}


/**
 * Adds default wrapper/pure declarations if they haven't been set already.
 *
 * Returns true on errors.
 */
bool TangerTransform::addDefaultTxnalWrapperDeclarations()
{
    FuncInfo *fi;
    // pure functions
    // TODO: if there are no asynchronous restarts, add EH ABI calls as tm_pure functions
    // as well (see Interface::isExceptionHandling())
    const int pureCount = 4;
    Interface::Functions pure[pureCount] = { Interface::assert_fail,
            Interface::tanger_stm_get_tx, Interface::itm_trycommit,
            Interface::itm_abort };
    for (int i = 0; i < pureCount; i++) {
        Function* f = tif.getFunction(pure[i]);
        fi = getOrCreateFuncInfo(f);
        if (!fi->hasBeenChecked) {
            fi->setPure(f);
            DEBUG(errs() << "Function " << f->getName() << " is pure\n");
        }
    }
    // wrapper functions
    const int wrapperCount = 4;
    Interface::Functions originals[wrapperCount] = { Interface::malloc,
            Interface::calloc, Interface::realloc, Interface::free };
    Interface::Functions wrappers[wrapperCount] = {
            Interface::itm_malloc, Interface::itm_calloc,
            Interface::tanger_stm_realloc, Interface::itm_free };
    for (int i = 0; i < wrapperCount; i++) {
        Function *original = tif.getFunction(originals[i]);
        Function *wrapper = tif.getFunction(wrappers[i]);
        fi = getOrCreateFuncInfo(original);
        if (!fi->hasBeenChecked) {
            fi->setWrapped(wrapper);
            // Mark target as txnal version
            getOrCreateFuncInfo(wrapper)->setTxnalVersion(wrapper, 0, original,
                    fi, true);
            DEBUG(errs() << "Function " << original->getName() << " has wrapper " << wrapper->getName() << "\n");
        }
    }
    addDefaultTxnalWrapperExceptionDeclarations();
    return false;
}

bool TangerTransform::addDefaultTxnalWrapperExceptionDeclarations()
{
    FuncInfo *fi;
    Function* f;

    // TODO these functions will probably need wrappers in the future
    const int funcPureCount = 8;
    const char* funcPure[funcPureCount] = {
            "__cxa_begin_catch", "__cxa_allocate_exception",
            "__cxa_throw", "_Unwind_Resume_or_Rethrow",
            "__cxa_call_unexpected", "__gxx_personality_v0",
            "__cxa_rethrow", "__cxa_end_catch"
    };
    for (int i = 0; i < funcPureCount; i++) {
        Function* f = M->getFunction(funcPure[i]);
        if (!f) continue;
        fi = getOrCreateFuncInfo(f);
        if (!fi->hasBeenChecked) {
            fi->setPure(f);
            DEBUG(errs() << "Function " << f->getName() << " is pure\n");
        }
    }

    const int pureCount = 2;
    Intrinsic::ID pure[pureCount] = {Intrinsic::eh_exception, Intrinsic::eh_selector};
    for (int i = 0; i < pureCount; i++) {
        f = tif.getIntrinsic(pure[i]);
        fi = getOrCreateFuncInfo(f);
        if (!fi->hasBeenChecked) {
            fi->setPure(f);
            DEBUG(errs() << "Function " << f->getName() << " is pure\n");
        }
    }

    return false;
}


/**
 * Makes function f callable.
 * Also makes all functions called by f callable.
 */
void TangerTransform::setCallable(Function *f, FuncInfo *fi,
        FunctionSet *functionsToCheck, bool potentially)
{
    if (fi->type == FuncInfo::Pure) return;
    assert(fi->type != FuncInfo::TxnalVersion);
    if (potentially && fi->type == FuncInfo::Callable) {
        // Callable overrides potentially callable
        potentially = false;
    }
    fi->type = potentially ? FuncInfo::PotentiallyCallable : FuncInfo::Callable;
    if (functionsToCheck && !fi->hasBeenChecked) functionsToCheck->insert(f);
    //DEBUG(errs() << (potentially ? "setPotentiallyCallable " : "setCallable ") << f->getName() << "\n");
    for (FunctionSet::iterator i = fi->levelZeroCallees.begin(),
            ie = fi->levelZeroCallees.end(); i != ie; ++i) {
        Function *cf = *i;
        FuncInfo *cfi = getOrCreateFuncInfo(cf);
        if (!(fi->hasBeenChecked && (cfi->type == FuncInfo::Callable
                || (potentially && cfi->type == FuncInfo::PotentiallyCallable)))) {
            // only propagate information if callee has not been checked yet and
            // is neither (1) callable nor (2) potentiallyCallable and we want to set to
            // potentiallyCallable
            setCallable(cf, cfi, functionsToCheck, potentially);
        }
    }
    // if function is callable, we don't need callees anymore
    if (!potentially) fi->levelZeroCallees.clear();
}


/**
 * Find all functions that contain calls to begin and commit functions.
 * Analyze them first.
 * Split BBs that contain txn demarcation calls (begin, commit, ...).
 */
bool TangerTransform::checkFunctionsWithTransactions()
{
    bool error = false;
    const int funcCount = 5;
    Interface::Functions funcs[funcCount] = { Interface::tanger_begin,
            Interface::tanger_commit, Interface::itm_begin,
            Interface::itm_commit, Interface::itm_trycommit };
    FunctionSet ptf;

    for (int jf = 0; jf < funcCount; jf++) {
        Function* bf = tif.getFunction(funcs[jf]);
        bool beginFunc = tif.isBegin(bf);
        Value::use_iterator j = bf->use_begin(), je = bf->use_end();
        for (; j != je; ++j) {
            CallInst* i = dyn_cast<CallInst> (*j);
            if (i && (i->getCalledFunction() == bf)) {
                // split the BB first. makes the analysis later on easier.
                BasicBlock *bb = i->getParent();
                bb->splitBasicBlock(++(BasicBlock::iterator(i)),
                        "postDemarcation");
                bb->splitBasicBlock(i, "demarcation");
                bb = i->getParent();
                FuncInfo *fi = getOrCreateFuncInfo(bb->getParent());
                if (beginFunc) {
                    // potentially a txnal function
                    ptf.insert(bb->getParent());
                    // store reference to begin call
                    fi->freeBegins[bb] = i;
                }
                else {
                    // store reference to commit call
                    fi->freeCommits[bb] = i;
                }
            }
            else {
                printWarning() << " " << bf->getName()
                        << " has a user that is not a direct call";
                j->dump();
                assert(0 && "something is probably wrong");
            }
        }
    }

    // check functions that might have txns. Only keep the good ones.
    for (FunctionSet::iterator i = ptf.begin(), ie = ptf.end(); i != ie; i++) {
        // Has the function be checked already?
        Function *f = *i;
        FuncInfo *fi = getOrCreateFuncInfo(f);
        if (fi->hasBeenChecked) continue;
        fi->hasBeenChecked = true;

        // check and determine BB props
        analyzeAndCheckFunction(*f, fi, &txnalFunctions);
        if (fi->checkResult == FuncInfo::NothingValid) {
            error = true;
            printError(*f)
                    << " function has transactions but cannot be transformed\n";
        }
        else levelOneFunctions.insert(f);
    }

    return error;
}


/**
 * Analyzes and checks a function.
 *
 * This consists of first checking whether txns are properly closed and
 * nested. We also detect which nesting level each basic block has.
 * Second, we check whether basic blocks can actually be transformed and thus
 * whether txns in the function and/or the whole function can be transformed.
 */
void TangerTransform::analyzeAndCheckFunction(Function& f, FuncInfo *fi,
        std::set<Function*>* txnalFunctions)
{
    DEBUG(errs() << "CHECKING function " << f.getName() << "\n");
    if (determineLevels(f, fi)) fi->checkResult = FuncInfo::NothingValid;
    else {
        // structure is okay, now check whether we can transform
        checkFunction(f, fi, txnalFunctions);
    }
    fi->hasBeenChecked = true;
    DEBUG(errs() << " check result: " <<
            (fi->checkResult == FuncInfo::CompletelyValid ? "complete" :
                    fi->checkResult == FuncInfo::TxnsValid ? "txns" :
                    fi->checkResult == FuncInfo::NothingValid ? "not valid" : "BUG") << "\n");
}


namespace {
struct BBProp {
    Transaction* txn;
    bool isBegin;
    bool isCommit;
    BBProp(Transaction* txn = 0, bool begin = 0, bool commit = 0) :
        txn(txn), isBegin(begin), isCommit(commit)
    {
    }
};
}

/**
 * Analyzes control flow to determine nesting level.
 */
bool TangerTransform::determineLevels(Function& F, FuncInfo* fi)
{
    typedef std::map<BasicBlock*, BBProp> BBPropMap;
    BBPropMap bbprops;
    std::vector<BasicBlock*> todo;

    bool error = false;
    DEBUG(errs() << " analyzing txns\n");

    todo.push_back(F.getBasicBlockList().begin());
    bbprops[todo.back()] = BBProp(0, false, false);
    // Traverse the CFG.
    // Because we split BBs with begin and commits before, the analysis gets
    // easier. BBs are either begin, commit, ordinary, or return blocks.
    // Also, there is always a nontxnal or parent block between two txns.
    while (!todo.empty()) {
        BasicBlock* bb = todo.back();
        todo.pop_back();
        BBProp prop = bbprops[bb];
        Transaction* txn = prop.txn;
        // is the block in a txn and a return block?
        if (txn && isa<ReturnInst> (bb->getTerminator())) {
            error = true;
            printError(F, *bb) << "return block is transactional\n";
        }
        // commit blocks: successors are associated with the parent txn
        if (prop.isCommit) {
            if (txn) txn = txn->parent;
        }
        // look at successor blocks
        for (succ_iterator i = succ_begin(bb), ie = succ_end(bb); i != ie; ++i) {
            BasicBlock* succBB = *i;
            BBPropMap::iterator succPropIt = bbprops.find(succBB);

            // if the block hasn't been seen before, just propagate information
            if (succPropIt == bbprops.end()) {
                std::map<BasicBlock*, Instruction*>::iterator it;
                // begin blocks start new txns
                if ((it = fi->freeBegins.find(succBB)) != fi->freeBegins.end()) {
                    Transaction* newTxn = fi->addTxn(it->second, txn,
                            cast<CallInst>(it->second)->getCalledFunction() == tif.getFunction(
                                    Interface::tanger_begin));
                    bbprops[succBB] = BBProp(newTxn, true, false);
                    fi->freeBegins.erase(it);
                }
                // commit blocks close txns and successors blocks
                else if ((it = fi->freeCommits.find(succBB))
                        != fi->freeCommits.end()) {
                    if (!txn) {
                        //error = true;
                        //printError(F, *bb)
                        // FIXME this doesn't work. it breaks the begin/commit check.
                        printWarning(F, *bb)
                                << "commit without a matching transaction begin\n";
                        bbprops[succBB] = BBProp(0, false, false);
                    }
                    else {
                        txn->commitCalls.push_back(it->second);
                        bbprops[succBB] = BBProp(txn, false, true);
                    }
                    fi->freeCommits.erase(it);
                }
                // ordinary block, just propagate information
                else {
                    bbprops[succBB] = BBProp(txn, false, false);
                    if (txn) txn->code.insert(succBB);
                }
                // process block
                todo.push_back(succBB);
            }

            // block has been seen before, check consistency
            else {
                BBProp& succProp = succPropIt->second;
                Transaction* mergeTxn = 0;
                // if the successor is a begin block, it must be properly nested.
                // perhaps merge current and other parent txn.
                if (succProp.isBegin) mergeTxn = succProp.txn->parent;

                // otherwise, the successor must belong to the same txn
                else mergeTxn = succProp.txn;

                // try to merge txns if necessary
                if (mergeTxn != txn) {
                    if (mergeTxn->parent != txn->parent) {
                        printError(F, *bb)
                                << "cannot merge (parent) transactions because they have different parents\n";
                        error = true;
                    }
                    else if (mergeTxn->level != txn->level) {
                        printError(F, *bb)
                                << "cannot merge (parent) transactions because they have different levels\n";
                        error = true;
                    }
                    else if (mergeTxn->markerCalls || txn->markerCalls) {
                        printError(F, *bb)
                                << "cannot merge (parent) transactions because at least one"
                                << "of them uses the old begin/commit marker calls\n";
                        error = true;
                    }
                    else {
                        printWarning(F, *bb) << "merging transactions\n";
                        // We can merge the two txns.
                        // rewrite bbprops
                        for (BBPropMap::iterator i = bbprops.begin(), ie =
                                bbprops.end(); i != ie; i++)
                            if (i->second.txn == mergeTxn) i->second.txn = txn;
                        // merge txn objects
                        fi->mergeAndDeleteTxn(txn, mergeTxn);
                    }
                }

            }
        }
    }

    // create a set of all remaining nontxnal blocks (level zero)
    if (!error) {
        for (Function::iterator i = F.begin(), ie = F.end(); i != ie; i++) {
            BBPropMap::iterator propIt = bbprops.find(i);
            if (propIt == bbprops.end()) {
                // We did not see this BB in our CFG traversal.
                if (pred_begin(i) == pred_end(i)) {
                    // No predecessor. This sometimes happens when debug
                    // information is included, and optimizations are disabled.
                    // If it has a single successor that has BBProps info,
                    // we make it belong to the txn.
                    TerminatorInst *ti = i->getTerminator();
                    if (ti->getNumSuccessors() == 1) {
                        propIt = bbprops.find(ti->getSuccessor(0));
                        if (propIt != bbprops.end()) {
                            if (!propIt->second.txn) fi->levelZeroCode.insert(i);
                            else propIt->second.txn->code.insert(i);
                        }
                    }
                }
                if (propIt == bbprops.end()) printWarning(F, *i) << "unhandled dead code\n";
            }
            else if (!propIt->second.txn) {
                fi->levelZeroCode.insert(i);
            }
        }
    }

    // check that we processed all begins and commits
    if (!fi->freeBegins.empty() || !fi->freeCommits.empty()) {
        printWarning(F)
                << "some begins/commits have not been processed, probably dead code\n";
    }

    return error;
}


/**
 * Checks a single basic block for whether it can be transactional or not.
 * hasToBeTxnal states whether the block has to be transactional or not.
 * If printWarnings is true, warnings for errors in blocks that don't have to be
 * transactional will be printed in debug mode.
 * Returns true iff it can be transactional.
 */
bool TangerTransform::checkBB(BasicBlock* bb, bool hasToBeTxnal, FuncInfo* fi,
        FunctionSet* txnalFunctions, bool printWarnings)
{
    bool error = false;
    for (BasicBlock::iterator jj = bb->begin(), jje = bb->end(), ii; jj != jje;) {
        ii = jj++;

        // handle calls and invokes
        if (isa<CallInst> (&*ii) || isa<InvokeInst> (&*ii)) {
            const char* err = 0;
            CallSite cs = CallSite::get(&*ii);
            Function *f = cs.getCalledFunction();
            // ignore bitcasts and constant expressions
            if (!f) f = dyn_cast<Function> (
                    cs.getCalledValue()->getUnderlyingObject());
            if (!f) {
                // check for inlines asm, which is not supported
                if (isa<InlineAsm> (cs.getCalledValue())
                        && !clResolveNontxnalDirectCalls) {
                    err = "no support for inline asm yet";
                }
                // we do handle indirect calls
            }
            // skip debug intrinsics
            else if (f->isIntrinsic() && isa<DbgInfoIntrinsic> (
                    cs.getInstruction())) {
            }
            // we handle memcpy/memmove/memset
            else if (isa<MemIntrinsic>(&*ii)) {
            }
            else {
                // No intrinsic, no STM function
                FuncInfo *targetFI = getOrCreateFuncInfo(f);
                //DEBUG(errs() << "Callee: " << f->getName() << " type: " << targetFI->type << "\n");
                // ignore if it's pure
                if (targetFI->type == FuncInfo::Pure) {}
                // calls txnal functions directly?
                // are there are weird wrapper definitions that can cause this?
                else if (targetFI->type == FuncInfo::TxnalVersion)
                    err = "calling txnal version directly";
                // if it has been marked as callable, someone else will have checked that it actually is
                else if (targetFI->type == FuncInfo::Callable) {}
                // if we cannot handle declaration-only functions, the call is an error
                else {
                    if (f->isDeclaration() && clWholeProgram
                            && !clResolveNontxnalDirectCalls)
                        err = "function has no body";
                    // Mark target as callable or as potentially callable
                    if (hasToBeTxnal)
                        setCallable(f, targetFI, txnalFunctions, false);
                    else {
                        if (fi->type == FuncInfo::Callable)
                            setCallable(f, targetFI, txnalFunctions, false);
                        else fi->levelZeroCallees.insert(f);
                    }
                }
            }
            // if there was an error, set flags and print error msg
            if (err) {
                if (hasToBeTxnal) {
                    printError(*bb->getParent(), *bb) << " txnal call to "
                            << (f ? f->getName() : "<unknown>") << "(...): "
                            << err << "\n";
                    error = true;
                }
                else {
                    if (printWarnings)
                    DEBUG(errs() << "  call to "<<(f ? f->getName() : "<unknown>")
                            <<"(...) prevents txnal version: " << err << "\n");
                }
            }
        }
        // TODO forbid vector load/stores and stores
    }
    return !error;
}


/**
 * After we have set nesting levels, run a second analysis and check
 * whether the transactional level requested is actually possible.
 * Assertions are also checked and handled here.
 */
void TangerTransform::checkFunction(Function& F, FuncInfo *fi,
        FunctionSet* txnalFunctions)
{
    DEBUG(errs() << " analyzing whether transformations are possible" << "\n");
    assert(fi->type != FuncInfo::TxnalVersion);
    bool error = false;
    bool noTxnalVersion = false;

    // check txnal code
    for (TransactionSet::iterator i = fi->txns.begin(), ie = fi->txns.end();
            i != ie; i++) {
        Transaction* txn = *i;
        for (BBSet::iterator bb = txn->code.begin(), be = txn->code.end();
                bb != be; bb++) {
            if (!checkBB(*bb, true, fi, txnalFunctions, true)) error = true;
        }
    }
    // check level zero code (is only txnal in a txnal version of the function)
    for (BBSet::iterator bb = fi->levelZeroCode.begin(),
            be = fi->levelZeroCode.end(); bb != be; bb++) {
        if (!checkBB(*bb, false, fi, txnalFunctions, !noTxnalVersion))
            noTxnalVersion = true;
    }
    // store result
    if (error) fi->checkResult = FuncInfo::NothingValid;
    else if (noTxnalVersion) fi->checkResult = FuncInfo::TxnsValid;
    else fi->checkResult = FuncInfo::CompletelyValid;
}


/**
 * Prepares transformation of a single transaction.
 * Creates additional codepaths if necessary. Creates additional load/store
 * instructions from virtual registers to ease the transformation and puts
 * pointers to them in the doNotTransform set.
 */
void TangerTransform::prepareTransformTransaction(Function& F, Transaction* tx,
        Transaction::CodePath* origCP, InstrSet& doNotTransform)
{
    // Find value users outside of the code and demote virtual regs to stack
    // slots. This makes changing the CFG later a lot easier. We also store
    // the new loads/stores because they don't have to be instrumented and can
    // be removed with the mem2reg pass after the transformations (this is
    // easier because mem2reg might change BBs).
    std::vector<Instruction*> doNotTransformOrig;
    for (BBSet::iterator bbi = origCP->bbs.begin(), bbe = origCP->bbs.end();
            bbi != bbe; bbi++) {
        for (BasicBlock::iterator i = (*bbi)->begin(), ie = (*bbi)->end();
                i != ie; i++) {
            bool externalUser = false;
            for (Value::use_iterator ui = i->use_begin(), ue = i->use_end();
                    ui != ue; ui++) {
                if (!origCP->bbs.count(cast<Instruction> (*ui)->getParent())) {
                    externalUser = true;
                    break;
                }
            }
            // If the instr has any user in an outside BB, demote it to a stack slot.
            if (externalUser) {
                AllocaInst* ai = DemoteRegToStack(*i);
                // All users of the new alloca are new loads/stores.
                for (Value::use_iterator ui = ai->use_begin(),
                        ue = ai->use_end(); ui != ue; ui++) {
                    assert((isa<LoadInst>(*ui) || isa<StoreInst>(*ui))
                            && "unexpected user after DemoteRegToStack");
                    doNotTransform.insert(cast<Instruction>(*ui));
                    doNotTransformOrig.push_back(cast<Instruction>(*ui));
                }
                stat_instrsDemToStack++;
            }
        }
    }

    // Transform begin instructions.
    std::map<BasicBlock*, BasicBlock*> beginToDefaultSucc;
    for (unsigned i = 0; i < tx->beginCalls.size(); i++) {
        CallInst* ci = cast<CallInst> (tx->beginCalls.at(i));
        // transform begin call
        // TODO add asm compiler barriers? or should we rather do this directly
        // in the front-end? We link and optimize .bc modules before this pass,
        // so doing it here might be too late anyway.
        Instruction *begin = transform.replaceBegin(ci);
        ++stat_begin;
        // rewrite unconditional branch to switch statement
        BranchInst* br = dyn_cast<BranchInst> (
                begin->getParent()->getTerminator());
        assert(br && br->isUnconditional());
        BasicBlock* succ = br->getSuccessor(0);
        Value *cpId = transform.insertGetABICodePathID(begin, br);
        br->eraseFromParent();
        SwitchInst* si = SwitchInst::Create(cpId, succ, 1, begin->getParent());
        beginToDefaultSucc[si->getParent()] = succ;
        tx->beginCalls[i] = begin;
    }

    // Clone code paths from original code.
    for (Transaction::CodePathSet::iterator i = tx->codePaths.begin(),
            ie = tx->codePaths.end(); i != ie; i++) {
        if (*i == origCP) continue;

        // copy all BBs of the code path
        ValueMap<const Value*, Value*> valueMap;
        for (BBSet::iterator bi = origCP->bbs.begin(), be = origCP->bbs.end();
                bi != be; bi++) {
            BasicBlock* newBB = CloneBasicBlock(*bi, valueMap,
                    (*i)->transformer->getBBSuffix(), (*bi)->getParent());
            valueMap[*bi] = newBB;
            (*i)->bbs.insert(newBB);
        }

        // Map from original defs to defs in the current codepath.
        for (BBSet::iterator bi = origCP->bbs.begin(), be = origCP->bbs.end();
                bi != be; bi++) {
            BasicBlock* cb = cast<BasicBlock> (valueMap[*bi]);
            for (BasicBlock::iterator bbi = (*bi)->begin(), bbe = (*bi)->end(),
                    cbbi = cb->begin(); bbi != bbe; bbi++, cbbi++) {
                for (unsigned op = 0, ope = bbi->getNumOperands(); op != ope; op++) {
                    const Value* def = bbi->getOperand(op);
                    Value* v = valueMap[def];
                    if (v != 0) cbbi->setOperand(op, v); // rewrite if def has a clone
                }
            }
        }

        // Clone doNotTransform information.
        // If we would keep a mapping from cloned instrs to the original code
        // path, transformers could also use this map to look up the real
        // doNotTransform information.
        for (std::vector<Instruction*>::iterator dnti = doNotTransformOrig.begin(),
                dnte = doNotTransformOrig.end(); dnti != dnte; dnti++) {
            Value* v = valueMap[*dnti];
            if (v) doNotTransform.insert(cast<Instruction>(v));
        }

        // Link additional code path by adding an additional case to the switch
        // statements of begin blocks.
        for (std::map<BasicBlock*, BasicBlock*>::iterator
                bi = beginToDefaultSucc.begin(),
                be = beginToDefaultSucc.end(); bi != be; bi++) {
            SwitchInst* si = cast<SwitchInst> (bi->first->getTerminator());
            BasicBlock* cpSucc = dyn_cast<BasicBlock> (valueMap[bi->second]);
            assert(cpSucc && "begin successor not cloned");
            si->addCase(ConstantInt::get(si->getContext(), APInt(32,
                    (*i)->transformer->getABIID())), cpSucc);
        }
    }

    // add stack save restore before the codepath switch statement
    //TODO How can we implement so that this really works?

    // Transform commit instructions.
    // We don't need to do anything special for commits because we replaced
    // all outgoing virtual regs with stack slots.
    assert(tx->commitCalls.size() > 0);
    for (unsigned i = 0; i < tx->commitCalls.size(); i++) {
        CallInst* ci = cast<CallInst> (tx->commitCalls.at(i));
        // transform begin call
        // TODO Add asm compiler barriers? See comments above.
        transform.replaceCommit(ci);
    }
}


/**
 * Transforms the function. Transforms level zero BBs too if required (ie, if
 * the function is a transactional version).
 */
void TangerTransform::transformFunction(Function& F, FuncInfo* fi,
        bool hasTransactions, bool createTxnalVersion)
{
    DEBUG(errs() << "TRANSFORMING function '" << F.getName()
            << "' hasTxns="<< hasTransactions << " txnalVer=" << createTxnalVersion << "\n");
    assert(fi->hasBeenChecked);

    InstrSet doNotTransform;

    // Insert getTxn instruction (we can do this before transformation
    // because it is pure).
    fi->tx = transform.insertGetTx(F.begin()->begin());

    // (1) prepare transformation of all txns
    if (hasTransactions) {
        for (TransactionSet::iterator ti = fi->txns.begin(),
                te = fi->txns.end(); ti != te; ti++) {
            // Create codepaths (original code becomes sequential code path).
            Transaction::CodePath* origCP = new Transaction::CodePath();
            origCP->transformer = &noChangeTransformer;
            origCP->bbs.insert((*ti)->code.begin(), (*ti)->code.end());
            (*ti)->code.clear();
            (*ti)->codePaths.insert(origCP);
            for (unsigned i = 0; i < transformers.size(); i++) {
                Transaction::CodePath* cp = new Transaction::CodePath();
                cp->transformer = transformers[i];
                (*ti)->codePaths.insert(cp);
            }
            // prepare transformation: clone codepaths, etc.
            prepareTransformTransaction(F, *ti, origCP, doNotTransform);
        }
    }

    // (2) Create txnal versions. We assume that nested txns get transformed
    // in the same way as outermost txns. Therefore, we can first prepare all
    // code paths and then clone and transform the clones.
    if (createTxnalVersion) {
        BBSet allCode;
        if (!hasTransactions) {
            for (Function::iterator i = F.begin(), ie = F.end(); i != ie; i++)
                allCode.insert(i);
        }
        for (unsigned tm = 0; tm < supportedTMs.size(); ++tm) {
            DEBUG(errs() << " creating clone for function '" << F.getName() << "'\n");

            // clone the function
            ValueMap<const Value*, Value*> valueMap;
            Function *ftxnal = getTxnalFunction(&F, supportedTMs[tm].first);
            Function::arg_iterator itxnal = ftxnal->arg_begin();
            for (Function::const_arg_iterator i = F.arg_begin(),
                    ie = F.arg_end(); i != ie; ++i, ++itxnal) {
                valueMap[i] = itxnal;
                itxnal->setName(i->getName());
            }
            SmallVector<ReturnInst*,2> ignoreReturns;
            // LLVM docs: If ModuleLevelChanges is false, VMap contains no non-identity GlobalValue mappings.
            // XXX: Not sure what that means exactly, true should be the conservative answer.
            CloneFunctionInto(ftxnal, &F, valueMap, true, ignoreReturns);
            // it should have already been set as txnal version
            assert(fi->getTxnalFunction(tm) == ftxnal);

            // clone func info
            FuncInfo* fitxnal = getOrCreateFuncInfo(ftxnal);
            // it should have already been set as txnal version
            assert(fitxnal->type == FuncInfo::TxnalVersion
                    && fitxnal->getOriginalFunction() == &F);

            // transform the clone
            Transformer* lzt = transformers[tm];
            if (hasTransactions) {
                // Transform all code paths. We iterate over the codepaths
                // in the original function, but the transformations will be
                // performed on the instructions mapped by valueMap
                for (TransactionSet::iterator ti = fi->txns.begin(),
                        te = fi->txns.end(); ti != te; ti++)
                    for (Transaction::CodePathSet::iterator
                            cpi = (*ti)->codePaths.begin(),
                            cpe = (*ti)->codePaths.end(); cpi != cpe; cpi++)
                        (*cpi)->transformer->transformCodePath((*cpi)->bbs,
                                doNotTransform, fi->tx, &valueMap);
                // transform level zero code
                lzt->transformCodePath(fi->levelZeroCode, doNotTransform,
                        fi->tx, &valueMap);
            }
            else {
                // does not contain txns, transform the whole code
                lzt->transformCodePath(allCode, doNotTransform, fi->tx,
                        &valueMap);
            }
        }
        fi->txnalVersionProcessed = true;
    }

    // (3) transform the codepaths of each txn. this happens after
    // preparing all txns and after creating txnal versions of the whole
    // function.
    for (TransactionSet::iterator ti = fi->txns.begin(), te = fi->txns.end();
            ti != te; ti++)
        for (Transaction::CodePathSet::iterator cpi = (*ti)->codePaths.begin(),
                cpe = (*ti)->codePaths.end(); cpi != cpe; cpi++)
            (*cpi)->transformer->transformCodePath((*cpi)->bbs, doNotTransform,
                    fi->tx);

    // (4) If there are no txns, remove the call to get the txn descriptor.
    // We needed it to create the txnal versions but don't need it in the
    // original function.
    if (!hasTransactions) {
        cast<Instruction> (fi->tx)->eraseFromParent();
        fi->tx = 0;
    }
}


/**
 * Returns or creates a txnal version of function f. The txnal function's name
 * must/will have the given suffix.
 */
Function* TangerTransform::getTxnalFunction(Function* f,
        std::string additional_suffix)
{
    // can't use getOrInsertFunction() here as is because according to the
    // docs, it doesn't return existing matching functions with internal
    // linkage
    std::string newname(tanger_txnal_func_prefix);
    newname += f->getName();
    newname += additional_suffix;

    Function* txf = M->getFunction(newname);
    if (!txf) {
        // function does not exist, create it
        txf = cast<Function> (M->getOrInsertFunction(newname,
                f->getFunctionType()));
        txf->copyAttributesFrom(f);
        return txf;
    }
    else {
        // Function exists, check type.
        // We could additionally check that the function is a declaration
        // only if it hasn't been created by the pass yet. This way, we
        // could detect collisions (which shouldn't exist if the pass is
        // run only once).
        if (txf->getType() == f->getType()) return txf;
    }
    errs() << "ERROR: collision for transactional function '" << newname
            << "': it exists with type ";
    txf->getFunctionType()->print(errs());
    errs() << " but we need type ";
    f->getFunctionType()->print(errs());
    errs() << "\n";
    fatalError();
    return f;
}

