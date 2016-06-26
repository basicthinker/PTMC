/* Copyright (C) 2007-2009  Torvald Riegel
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
#ifndef GIBRALTAR_H_
#define GIBRALTAR_H_

//STATISTIC(tanger_stat_gibraltar_startstop, "Number of Gibraltar regions");
//cl::opt<bool>
//clGibraltar("tanger-gibraltar", cl::init(false), cl::desc("Enables Gibraltar transformation"));
//cl::opt<bool>
//clGibraltarOnly("tanger-gibraltaronly", cl::init(false), cl::desc("Delegates all instrumentation to Gibraltar"));
//cl::opt<bool>
//clGibraltarStackHints("tanger-gibraltar-stackhints", cl::init(true), cl::desc("Gives stackframe context information to Gibraltar"));

// module init
//if (!clWholeProgram && clGibraltar)
//    printWarning() << "Enabling Gibraltar support will lead to Gibraltar always handling calls to external functions\n";
//// If we only use Gibraltar, we need no indirect call magic
//if (clGibraltarOnly) {
//    if (clIndirectAuto) {
//        printWarning() << "Disabling auto-detection of indirect call targets (Gibraltar handles everything)\n";
//        clIndirectAuto = false;
//    }
//}

//processModule:
//// other things
//for (std::vector<CallInst*>::iterator i = initCalls.begin(), ie = initCalls.end(); i != ie; i++) {
//    if(clGibraltar || clGibraltarOnly) transform.insertGibraltarInit(*i);
//}

// checkFunction, vor inlineasm checks etc.
//if (clGibraltarOnly) {} // skip other checks if we delegate to Gibraltar
//if (clGibraltar) {} // inline asm calls are delegated to Gibraltar
// weiter hinten, nach checkFunctionIgnore()
//// skip further checks if we delegate to Gibraltar
//else if (clGibraltarOnly) {}
// weiterhin: bei if (isDeclaration()) check: gibraltar kann das

/**
 * TODO:
 * - threadinit is now lazily performed in getTxnContext. we need a hook or sth to let the TM trigger gibraltar thread init.
    // thread initialization
    std::vector<CallInst*> threadInitCalls;
    transform.getCalls(tif.getFunction(Interface::tanger_thread_init), threadInitCalls);
    for (std::vector<CallInst*>::iterator i = threadInitCalls.begin(), ie = threadInitCalls.end(); i != ie; i++) {
        if(clGibraltar || clGibraltarOnly) transform.insertGibraltarThreadInit(*i);
    }
 */

// in determineLevels:
//                else if (isa<AllocaInst>(&*i)) {
//                    /* This checks whether the address of stack objects is
//                     * taken and could therefore potentially leak to other
//                     * threads. Knowing that no stack address could leak out
//                     * can speedup Gibraltar instrumentation.
//                     * TODO Checking this here is a hack. Actually, it should
//                     * be checked for each txn (eg, in checkFunction()).
//                     */
//                    stackMightBeShared = true;
//                }


// in checkFunction:
//                bool checkStackMightBeShared = (clGibraltar || clGibraltarOnly) && fi->stackMightBeShared;
//
//                // check whether alloca is used by called function that might
//                // get inlined later. It is probably unlikely that LLVM will
//                // inline it later, but it is safer to be conservative here.
//                // Note that we only check blocks within txns; Level 0 code will
//                // only be executed in txnal versions, which have their own stack
//                // frame anyway.
//                if (checkStackMightBeShared && level) {
//                    for (Instruction::op_iterator it = ii->op_begin(), ite = ii->op_end(); it != ite; ++it) {
//                        Value *v = *it;
//                        while (true) {
//                            if (BitCastInst* bci = dyn_cast<BitCastInst>(v)) v = bci->getOperand(0);
//                            else if (ConstantExpr* ce = dyn_cast<ConstantExpr>(v)) v = ce->getOperand(0);
//                            else break;
//                        }
//                        if (isa<AllocaInst>(v)) {
//                            DEBUG(errs() << " Gibraltar: txnal instruction " << *ii << " uses alloca " << *v << "\n");
//                            stackMightBeShared = true;
//                            checkStackMightBeShared = false;
//                        }
//                    }
//                }

// im transformFunction, fuer nicht-transactionale bloecke: fuer tif.isBegin():
// it ist instruktion nach dem begin callInst
//                        if (clGibraltarOnly)
//                            transform.insertGibraltarStart(it, !fi->stackMightBeShared && clGibraltarStackHints);

//void TangerTransform::transformInstructions(Function& F, Module& M, FuncInfo* fi, BasicBlock *bb,
//    BasicBlock::iterator ii, BasicBlock::iterator iie, bool statsBeginCommit, BBProp *prop, int minLevel)
//{
//    std::set<Value *> allocatedExceptions = std::set<Value *>();
//
//    // iterate over instructions
//    for (BasicBlock::iterator iinext; ii != iie; ii=iinext) {
//        iinext = ii;
//        iinext++;
//
//        // calls
//        if (isa<CallInst>(&*ii)) {
//            CallInst* ci = dyn_cast<CallInst>(&*ii);
//            Function *f = ci->getCalledFunction();
//            if (!f) {
//                if (!clGibraltarOnly) {
//                    if (isa<InlineAsm>(ci->getCalledValue()) && clGibraltar) {
//                        // instrument inline asm using Gibraltar
//                        transform.insertGibraltarStartStop(ii,
//                                (minLevel == 0 || !fi->stackMightBeShared) && clGibraltarStackHints);
//                        tanger_stat_gibraltar_startstop++;
//                    }
//                    else {
//                        // handle calls that are not direct calls
//                        // FIXME this should start to use Gibraltar if the indirect call cannot be resolved
//                        if(prop->txnNature > -1 ) {
//                            Value *newtarget = transformGetIndirectCallTarget(ci,fi,prop->txnNature,!clMultipleTMSupport.empty());
//                            ci->setOperand(0, newtarget);
//                        }
//                    }
//                }
//            }
//            // begin
//            else if (tif.isBegin(f)) {
//                if (!clFlatNesting) {
//                    //assert(&*(++ii) == bb->getTerminator() && "begin() not at end of BB");
//                    if (clGibraltarOnly) {
//                        // stop Gibraltar before begin and start it again after begin
//                        transform.insertGibraltarStop(ii);
//                        transform.insertGibraltarStart(iinext,
//                                (minLevel == 0 || !fi->stackMightBeShared) && clGibraltarStackHints);
//                    }
//                    transformInstructionBegin(ci, fi, clUseIntelABI, prop);
//                    if (statsBeginCommit) ++tanger_stat_begin;
//                    break; // we have finished the block (begin() must be at the end of BB's)
//                }
//                else {
//                    // flat nesting required, transform to noop
//                    transform.replaceWithNoop(ci);
//                }
//            }
//            // commit
//            else if (tif.isCommit(f)) {
//                // even with flat nesting, we need to transform outermost commits
//                if (!clFlatNesting || prop->level <= minLevel) {
//                    if (clGibraltarOnly) {
//                        // stop Gibraltar before begin
//                        transform.insertGibraltarStop(ii);
//                        // and start it again after begin, if nested
//                        if (prop->level > minLevel) transform.insertGibraltarStart(iinext,
//                                (minLevel == 0 || !fi->stackMightBeShared) && clGibraltarStackHints);
//                    }
//                    if(tif.getFunction(Interface::itm_trycommit) == f)
//                        transform.replaceTryCommit(ci, prop->staticTxnID);
//                    else
//                        transform.replaceCommit(ci, prop->staticTxnID);
//                    if (statsBeginCommit) ++tanger_stat_commit;
//                }
//                else {
//                    // flat nesting required, transform to noop
//                    transform.replaceWithNoop(ci);
//                }
//            }
//            // replace all abort calls
//            else if (tif.isAbort(f)) {
//                transform.replaceAbort(ci, fi->tx);
//            }
//            // no further instrumentation if we delegate to Gibraltar
//            else if (clGibraltarOnly) {}
//            // keep debug info
//            else if (isa<DbgInfoIntrinsic>(ci)) {}
//            // ignore exception handling specific calls
//            else if (tif.isExceptionHandling(f)) {
//                if(f->getName().find("__cxa_allocate_exception") == 0) {
//                    for (Value::use_iterator i = ci->use_begin(), e = ci->use_end(); i != e; ++i)
//                      if (BitCastInst *Inst = dyn_cast<BitCastInst>(*i)) {
//                          for (Value::use_iterator ii = Inst->use_begin(), ie = Inst->use_end(); ii != ie; ++ii)
//                          allocatedExceptions.insert(*ii);
//                      }
//                }
//            }
//            // calls to other functions
//            // anything else: checking phase should have set a txnal version
//            else {
//                if(prop->txnNature > -1) {
//                    FuncInfo *targetFI = getOrCreateFuncInfo(f);
//                    assert(targetFI->type == FuncInfo::Callable || targetFI->type == FuncInfo::Pure);
//                    // create otherVersion if it hasn't been created yet
//                    assert(prop->txnNature != -2 && "TxnNature is not set");
//                    if (!targetFI->getTxnalFunction(prop->txnNature)) {
//                        assert(targetFI->type == FuncInfo::Callable);
//                        if (targetFI->checkResult == FuncInfo::CompletelyValid)
//                            targetFI->setTxnalCopy(getTransactionalFunction(M, f, SupportedTMs[prop->txnNature]), prop->txnNature);
//                        else if (!clGibraltar && !clResolveNontxnalDirectCalls) {
//                            // redirect to dummy (will create linker error)
//                            DEBUG(errs() << "  redirecting call to "<<f->getName()<<"(...) to txnal version that doesn't exist\n");
//                            targetFI->setTxnalCopy(getTransactionalFunction(M, f, SupportedTMs[prop->txnNature]), prop->txnNature);
//                        }
//                    }
//                    // otherVersion contains either same function (if pure) or txnal version
//                    if (!targetFI->getTxnalFunction(prop->txnNature)) {
//                        if (clGibraltar) {
//                            // insert Gibraltar activation for calls to external functions
//                            transform.insertGibraltarStartStop(ii,
//                                    (minLevel == 0 || !fi->stackMightBeShared) && clGibraltarStackHints);
//                            tanger_stat_gibraltar_startstop++;
//                        }
//                        else if (clResolveNontxnalDirectCalls) {
//                            // call resolve library function (might switch to serial mode at runtime)
//                            if(prop->txnNature > -1) {
//                                DEBUG(errs() << "  redirecting call to "<<f->getName()<<"(...) to resolver\n");
//                                Value *newf = transformGetIndirectCallTarget(ci,fi,prop->txnNature,!clMultipleTMSupport.empty());
//                                ci->setOperand(0, newf);
//                            }
//                        }
//                        else assert(clGibraltar || clResolveNontxnalDirectCalls);
//                    }
//                    else {
//                        //DEBUG(errs() << "  redirecting call to "<<f->getName()<<"(...) to txnal version\n");
//
//                        ci->setOperand(0, targetFI->getTxnalFunction(prop->txnNature));
//                        ++tanger_stat_redirected_calls;
//                    }
//                }
//            }
//        }
//        else if (clGibraltarOnly) {} // no further instrumentation if we delegate to Gibraltar
//
//        // load/store
//        else if (LoadInst *li = dyn_cast<LoadInst>(&*ii)) {
//            if (li->isVolatile()
//                    && getTxnNature(li, fi) >= 0) {
//                // loading from a volatile isn't common in sequential code,
//                // but in concurrent code...
//                printWarning(F,*bb)<<"load from volatile location\n";
//            }
//            transformInstructionsLoad(li, fi);
//        }
//        else if (StoreInst *si = dyn_cast<StoreInst>(&*ii)) {
//            if (si->isVolatile()
//                    && getTxnNature(si, fi) >= 0) {
//                // storing to a volatile isn't common in sequential code,
//                // but in concurrent code...
//                printWarning(F,*bb)<<"store to volatile location\n";
//            }
//            // only store if the target is not an allocated exception
//            if (allocatedExceptions.find(si) == allocatedExceptions.end())
//                transformInstructionsStore(si, fi);
//        }
//
//        // misc
//        else if (InvokeInst* iv = dyn_cast<InvokeInst>(&*ii)) {
//            Function * f = iv->getCalledFunction();
//            if (tif.isExceptionHandling(f)) {}
//            else {
//                FuncInfo *targetFI = getOrCreateFuncInfo(f);
//                DEBUG(errs() << "Function: " << f->getName() << "\n");
//                assert(targetFI->type == FuncInfo::Callable || targetFI->type == FuncInfo::Pure);
//                // create otherVersion if it hasn't been created yet
//                if (!targetFI->getTxnalFunction(prop->txnNature)) {
//                    assert(targetFI->type == FuncInfo::Callable);
//                    if (targetFI->checkResult == FuncInfo::CompletelyValid)
//                        targetFI->setTxnalCopy(getTransactionalFunction(M, f, SupportedTMs[prop->txnNature]),prop->txnNature);
//                    else if (!clGibraltar && !clResolveNontxnalDirectCalls) {
//                        // redirect to dummy (will create linker error)
//                        DEBUG(errs() << "  redirecting call to "<<f->getName()<<"(...) to txnal version that doesn't exist\n");
//                        targetFI->setTxnalCopy(getTransactionalFunction(M, f, SupportedTMs[prop->txnNature]),prop->txnNature);
//                    }
//                }
//                // otherVersion contains either same function (if pure) or txnal version
//                if (!targetFI->getTxnalFunction(prop->txnNature)) {
//                    if (clGibraltar) {
//                        // insert Gibraltar activation for calls to external functions
//                        transform.insertGibraltarStartStop(ii,
//                                (minLevel == 0 || !fi->stackMightBeShared) && clGibraltarStackHints);
//                        tanger_stat_gibraltar_startstop++;
//                    }
//                    else if (clResolveNontxnalDirectCalls) {
//                        // call resolve library function (might switch to serial mode at runtime)
//                        DEBUG(errs() << "  redirecting call to "<<f->getName()<<"(...) to resolver\n");
//                        Value *newf = transformGetIndirectInvokeTarget(iv);
//                        iv->setOperand(0, newf);
//                    }
//                    else assert(clGibraltar || clResolveNontxnalDirectCalls);
//                }
//                else {
//                    //DEBUG(errs() << "  redirecting call to "<<f->getName()<<"(...) to txnal version\n");
//                    iv->setOperand(0, targetFI->getTxnalFunction(prop->txnNature));
//                    //ci->setOperand(0, targetFI->otherVersion);
//                    ++tanger_stat_redirected_calls;
//                }
//            }
//        }
//    }
//}


// Transformations:
//    void insertGibraltarInit(Instruction *initInstr);
//    void insertGibraltarThreadInit(Instruction *threadInitInstr);
//    void insertGibraltarStartStop(BasicBlock::iterator& it, bool txnlocalStackFrame = false);
//    void insertGibraltarStart(BasicBlock::iterator& it, bool txnlocalStackFrame = false);
//    void insertGibraltarStop(BasicBlock::iterator& it);
//    Function* func_gibraltar_init;
//    Function* func_gibraltar_thread_init;
//    Function* func_gibraltar_execute_instrumented;
//    Function* func_gibraltar_execute_instrumented_txnlocalstackframe;
//    Function* func_gibraltar_execute_uninstrumented;
//    // Gibraltar
//    func_gibraltar_init = getOrCreateFunction("tanger_stm_gibraltar_init", ftype_hook);
//    func_gibraltar_thread_init = getOrCreateFunction("tanger_stm_gibraltar_thread_init", ftype_hook);
//    func_gibraltar_execute_instrumented = getOrCreateFunction("tanger_stm_gibraltar_execute_instrumented", ftype_hook);
//    func_gibraltar_execute_instrumented_txnlocalstackframe = getOrCreateFunction("tanger_stm_gibraltar_execute_instrumented_txnlocalstackframe", ftype_hook);
//    func_gibraltar_execute_uninstrumented = getOrCreateFunction("tanger_stm_gibraltar_execute_uninstrumented", ftype_hook);

///**
// * Inserts Gibraltar init and threadInit after STM init calls.
// */
//void Transformations::insertGibraltarInit(Instruction* initInstr)
//{
//    BasicBlock::iterator ii = initInstr;
//    ii++;
//    CallInst* i = CallInst::Create(func_gibraltar_init, "", ii);
//    i->setCallingConv(CallingConv::C);
//    i->setTailCall(false);
//}
//
///**
// * Inserts Gibraltar init and threadInit after STM init calls.
// */
//void Transformations::insertGibraltarThreadInit(Instruction* threadInitInstr)
//{
//    BasicBlock::iterator ii = threadInitInstr;
//    ii++;
//    CallInst* i = CallInst::Create(func_gibraltar_thread_init, "", ii);
//    i->setCallingConv(CallingConv::C);
//    i->setTailCall(false);
//}
//
//void Transformations::insertGibraltarStartStop(BasicBlock::iterator& it, bool txnlocalStackFrame)
//{
//    BasicBlock::iterator it2 = it++;
//    insertGibraltarStart(it2, txnlocalStackFrame);
//    insertGibraltarStop(it);
//}
//
//void Transformations::insertGibraltarStart(BasicBlock::iterator& it, bool txnlocalStackFrame)
//{
//    CallInst* i = CallInst::Create(txnlocalStackFrame ?
//            func_gibraltar_execute_instrumented_txnlocalstackframe : func_gibraltar_execute_instrumented, "", it);
//    i->setCallingConv(CallingConv::C);
//    i->setTailCall(false);
//}
//
//void Transformations::insertGibraltarStop(BasicBlock::iterator& it)
//{
//    CallInst* i = CallInst::Create(func_gibraltar_execute_uninstrumented, "", it);
//    i->setCallingConv(CallingConv::C);
//    i->setTailCall(false);
//}


#endif /* GIBRALTAR_H_ */
