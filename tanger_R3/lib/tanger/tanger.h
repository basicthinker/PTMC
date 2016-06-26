/* Copyright (C) 2007-2010  Torvald Riegel
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
#ifndef TANGER_PASS_H_
#define TANGER_PASS_H_

#include "llvm/Pass.h"
#include "llvm/Module.h"
#include "interface.h"
#include "transformations.h"
#include "funcinfo.h"
#include <set>
#include <cstring>

namespace tanger {

/**
 * Transactifies a module.
 *
 * We need a module pass because function passes cannot add functions to the
 * module.
 */
class TangerTransform: public ModulePass, public FuncInfoProvider {
public:
    static char ID;
protected:
    Interface tif;
    Transformations transform;
public:
    TangerTransform(char& id = ID);

    virtual bool runOnModule(Module& M);
    virtual void getAnalysisUsage(AnalysisUsage &AU) const;

protected:
    /// Nontransactional functions that contain transactions.
    FunctionSet levelOneFunctions;
    // Functions called from within transactions.
    FunctionSet txnalFunctions;
    /// A map from nontxnal function to txnal version for all indirect call targets.
    FunctionSet potentialIndirectCallTargets;

    typedef std::map<Function*, FuncInfo*> FuncInfoMap;
    FuncInfoMap funcInfos;

    // Set of supported TM transformations/implementations.
    std::vector<std::pair<std::string, uint32_t> > supportedTMs;
    std::vector<Transformer*> transformers;
    NoChangeTransformer noChangeTransformer;

    /// The module that is transformed by this pass.
    Module* M;

    void processModuleInit();
    void processModule();

    virtual FuncInfo* getOrCreateFuncInfo(Function* f);

    bool createDeclaredTxnalWrapper(Function* original, std::string wrapperName);
	bool findTxnalWrapperDeclarations();
    bool addDefaultTxnalWrapperDeclarations();
    bool addDefaultTxnalWrapperExceptionDeclarations();
    bool checkFunctionsWithTransactions();
    void analyzeAndCheckFunction(Function& f, FuncInfo *fi,
            FunctionSet* txnalFunctions);
    bool determineLevels(Function& F, FuncInfo *fi);
    void checkFunction(Function& F, FuncInfo* fi, FunctionSet* txnalFunctions);
    bool checkBB(BasicBlock* bb, bool hasToBeTxnal, FuncInfo* fi,
            FunctionSet* txnalFunctions, bool printWarnings);
    void setCallable(Function *f, FuncInfo *fi, FunctionSet *functionsToCheck,
            bool potentially);

    void transformFunction(Function& F, FuncInfo *fi, bool hasTransactions,
            bool createTxnalVersion);
    void prepareTransformTransaction(Function& F, Transaction* tx,
            Transaction::CodePath* origCP, InstrSet& doNotTransform);

    void loadTMConfig(const char *fileName);

    Function* getTxnalFunction(Function* f, std::string additional_suffix);
};


} //namespace tanger

#endif /*TANGER_PASS_H_*/
