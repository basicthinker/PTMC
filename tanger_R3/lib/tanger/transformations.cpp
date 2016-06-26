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
/**
 * Low-level code for transformations.
 */
#define DEBUG_TYPE "tanger"

#include <stdarg.h>
#include "llvm/DerivedTypes.h"
#include "llvm/Constants.h"
#include "llvm/Intrinsics.h"
#include "llvm/Instructions.h"
#include "llvm/Support/Debug.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Support/CallSite.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/IRBuilder.h"

#include "transformations.h"
#include "support.h"
#include "buildinfo.inc"

namespace {

cl::opt<bool> clABILSFuncs("tanger-abi-ls-funcs", cl::init(true),
        cl::desc("Use ABI load/store functions"),
        cl::Hidden);

cl::opt<bool> clAlignedLSFuncs("tanger-aligned-ls-funcs", cl::init(true),
        cl::desc("Call special load/store functions if addr is aligned"),
        cl::Hidden);

cl::opt<bool> clAlwaysInlineLSFuncs("tanger-ls-alwaysinline", cl::init(true),
        cl::desc("Try to always inline load/store functions"), cl::Hidden);

cl::opt<bool> clAddCallToTMShutdown("tanger-add-shutdown-call", cl::init(false),
        cl::desc("Insert a global destructor that calls the TM's shutdown function"), cl::Hidden);

cl::opt<bool> clStackSaveHack("tanger-stacksavehack", cl::init(true),
        cl::desc("Use the stack save/restore hack"), cl::Hidden);
cl::opt<unsigned> clSetjmpBufSize("tanger-jmpbufsize", cl::init(64),
        cl::desc("Size of the setjmp buffer in bytes"), cl::Hidden);

STATISTIC(stat_loads, "Number of loads replaced");
STATISTIC(stat_stores, "Number of stores replaced");
STATISTIC(stat_instrsNotTransformed, "Number of instrs that were not transformed because Tanger created them");
STATISTIC(stat_redirectedCalls, "Number of redirected calls");
STATISTIC(stat_resolvedDirectCalls, "Number of direct calls that might trigger serial mode");
STATISTIC(stat_resolvedIndirectCalls, "Number of indirect calls or calls to asm() in txnal code");

}

using namespace tanger;


bool Transformations::init(Interface* tif)
{
    this->tif = tif;
    this->M = tif->getModule();
    this->TD = tif->getTargetData();
    DEBUG(errs() << "initializing transformations\n");

    const Type* type_voidptr = tif->getTypeVoidPtr();

    // functions
    func_get_tx = tif->getFunction(Interface::tanger_stm_get_tx);
    func_save_restore_stack = tif->getOrCreateFunction(
            tif_tanger_stm_save_restore_stack, type_voidptr, type_voidptr,
            type_voidptr, (const Type*) 0);
    func_begin = tif->getFunction(Interface::itm_begin);
    func_commit = tif->getFunction(Interface::itm_commit);

    return !hadFatalError();
}


Transformations::~Transformations() {}


/**
 * Replaces all calls to function f with noop instructions.
 */
void Transformations::removeCalls(Function* f)
{
    std::vector<CallInst*> calls;
    getCalls(f, calls);
    for (std::vector<CallInst*>::iterator i = calls.begin(), ie = calls.end();
            i != ie; i++) {
        CallInst* ci = *i;
        replaceWithNoop(ci);
    }
}


Value* Transformations::insertGetTx(Instruction* insertBefore)
{
    CallInst* ptr =
            CallInst::Create(func_get_tx, "tanger_var_tx", insertBefore);
    tif->setCallProps(ptr);
    return ptr;
}


/**
 * Because transactions can be aborted anywhere and accesses to stack slots
 * are not instrumented, stack slots that are live into the txn must not be
 * reused or overwritten until the txn has committed.
 * TODO Replace the setjmp call with a returns_twice attribute on begin(),
 * once LLVM has support for this. Also, add a returns_twice ends to commit
 * calls to decrease overhead for the nontxnal parts of the function.
 *
 * If the stack save/restore hack is enabled, begin() is replaced with the
 * following code:
 * (1) We get stack pointer and frame pointer using LLVM intrinsics.
 * (2) We let the STM prepare an area where we can store the contents of our
 * strack frame. The area that we save or restore is [SP, FP). Assumptions:
 * Stack grows downwards, SP upwards holds all local variables, and the stack
 * from at FP or larger addresses (old frame pointer, return address, ...) is
 * not modified.
 * (3) We call the STM's begin() after stack save/restore.
 */
CallInst* Transformations::replaceBegin(CallInst* ci)
{
    CallInst* b = 0;
    // replace begin
    assert(ci->getNumUses() == 0);

    if (clStackSaveHack) {
        // get SP and FP, call prepare stack, add branch
        // TODO portability: we assume that sp < fp (sp==low_addr, fp==high_addr)
        Module* M = ci->getParent()->getParent()->getParent();
        Function* func_llvm_fp =
                Intrinsic::getDeclaration(M, Intrinsic::frameaddress);
        Function* func_llvm_sp = Intrinsic::getDeclaration(M, Intrinsic::stacksave);
        CallInst* fp = CallInst::Create(func_llvm_fp,
                Constant::getNullValue(IntegerType::get(M->getContext(), 32)),
                "fp", ci);
        CallInst* sp = CallInst::Create(func_llvm_sp, "sp", ci);

        std::vector<Value*> gsa_params;
        gsa_params.push_back(sp);
        gsa_params.push_back(fp);
        CallInst* gsa = CallInst::Create(func_save_restore_stack,
                gsa_params.begin(), gsa_params.end(), "", ci);
        tif->setCallProps(gsa);
    }
    else {
        // The setjmp call will prevent stack slot coloring.
        // Create a setjmp buffer at the start of the function.
        Function* f = ci->getParent()->getParent();
        AllocaInst* ai = new AllocaInst(
                ArrayType::get(Type::getInt8Ty(ci->getContext()),
                        clSetjmpBufSize), "",
                f->getEntryBlock().getFirstNonPHI());
        Value* aiv = new BitCastInst(ai, Type::getInt8PtrTy(ci->getContext()), "", ci);
        // We cannot use the setjmp intrinsic function because current LLVM
        // requires the functions name to be "setjmp".
        Function* sjf = tif->getOrCreateFunction("setjmp",
                Type::getInt32Ty(ci->getContext()),
                Type::getInt8PtrTy(ci->getContext()), NULL);
        CallInst::Create(sjf, aiv, "", ci);
    }

    b = CallInst::Create(func_begin,
            ConstantInt::get(M->getContext(), APInt(32, 0, false)), "", ci);
    tif->setCallProps(b);
    ci->eraseFromParent();
    return b;
}


CallInst* Transformations::replaceCommit(CallInst* ci)
{
    // only replace old commit marker calls. for ABI commit/trycommit, only
    // set the call properties.
    if (ci->getCalledFunction() == tif->getFunction(Interface::tanger_commit)) {
        CallInst* c = CallInst::Create(func_commit, "", ci);
        tif->setCallProps(c);
        ci->replaceAllUsesWith(c);
        ci->eraseFromParent();
        return c;
    }
    else tif->setCallProps(ci);
    return ci;
}


/**
 * Replaces an instruction with a noop instruction.
 * The instruction must not have any users.
 */
void Transformations::replaceWithNoop(Instruction* i)
{
	// We add a fake noop before removing the instruction to avoid having
	// BBs that just contain terminator instructions
	// We actually don't want to replace uses.
	// If we replace them, we could get a null value during runtime
	// instead of a compile time assertion failure.
	new BitCastInst(Constant::getNullValue(IntegerType::get(M->getContext(), 8)),
		IntegerType::get(M->getContext(), 8) , "noop", i);
	i->getParent()->getInstList().erase(i);
}


/**
 * Extracts the code path ID from the return value of beginTransaction().
 */
Value* Transformations::insertGetABICodePathID(Value* beginRetVal,
        Instruction* insertBefore)
{
    // bits 2 and 3 are used for stack save/restore, bit 4 for aborting a
    // transaction. remove them.
    ConstantInt* const_int32_18 = ConstantInt::get(insertBefore->getContext(),
            APInt(32, ~((uint32_t) (0x4 + 0x8 + 0x10))));
    BinaryOperator * bo = BinaryOperator::Create(Instruction::And, beginRetVal,
            const_int32_18, "", insertBefore);
    return bo;
}


/**
 * Returns all uses of a function that are call instructions.
 */
void Transformations::getCalls(Constant* constant,
        std::vector<CallInst*>& calls)
{
    Function* f;
    if (!(f = dyn_cast<Function> (constant))) {
        f = dyn_cast<Function> (constant->getUnderlyingObject());
        assert(f && "cannot get the function the constant expression is expected to refer to");
    }
    calls.clear();
    CallInst * ci;
    for (Value::use_iterator i = f->use_begin(), ie = f->use_end(); i != ie; ++i) {
        if ((ci = dyn_cast<CallInst> (*i))) {
            calls.push_back(ci);
        }
    }
}


/**
 * Inserts code to resolve the real target of an indirect call or a call
 * to asm code. Returns the real target value (i.e., what the transformed
 * call should actually use).
 */
Value* Transformations::getIndirectCallTarget(CallSite& cs, Interface* tif,
        unsigned internalID, uint32_t abiID)
{
    // we must not try to take the address of an intrinsic function or inline asm
    Function *f = cs.getCalledFunction();
    if (!f) {
        Value* v = cs.isCall() ?
                cast<CallInst> (cs.getInstruction())->getUnderlyingObject() :
                cast<InvokeInst> (cs.getInstruction())->getUnderlyingObject();
        f = dyn_cast<Function> (v);
    }
    if ((f && f->isIntrinsic()) || (isa<InlineAsm> (cs.getCalledValue()))) {
        // if this is inline asm or an intrinsic, we cannot resolve it anyway,
        // so resolve NULL and call the original code
        Value* v = ConstantPointerNull::get(tif->getTypeVoidPtr());
        std::vector<Value*> arg;
        arg.push_back(v);
        arg.push_back(ConstantInt::get(tif->getModule()->getContext(),
                APInt(32, 0)));
        CallInst* r = CallInst::Create(tif->getFunction(
                Interface::tanger_stm_indirect_resolve_multi),
                arg.begin(), arg.end(), "", cs.getInstruction());
        tif->setCallProps(r);
        return cs.getCalledValue();
    }
    else {
        // it is a function
        // get target via call to the extended STM's resolver function
        std::vector<Value*> arg;
        Value* v = new BitCastInst(cs.getCalledValue(),
                tif->getTypeVoidPtr(), "", cs.getInstruction());
        arg.push_back(v);
        arg.push_back(ConstantInt::get(tif->getModule()->getContext(),
                APInt(32, internalID)));
        v = CallInst::Create(tif->getFunction(
                Interface::tanger_stm_indirect_resolve_multi),
                arg.begin(), arg.end(), "", cs.getInstruction());
        tif->setCallProps(cast<CallInst> (v));
        v = new BitCastInst(v, cs.getCalledValue()->getType(),
                "", cs.getInstruction());
        return v;
    }
}


/**
 * Insert code that initializes the mapping from nontransactional functions
 * to transactional versions.
 * Currently, we initialize one table for the mapping to the transactional
 * version for the default kind of instrumentation, and another table
 * with mappings for all transactional versions if required.
 */
void Transformations::insertIndirectCallInit(
		Instruction* initInstr, std::map<Function*,
		std::vector<Function*> > nontxnalToTxnal)
{
	const Type* voidptr = PointerType::getUnqual(IntegerType::get(
            M->getContext(), 8));
	// we support multiple txnal versions, so we create the extended tables
	std::vector<Value*> arg;
	arg.push_back(ConstantInt::get(M->getContext(),
	        APInt(32, nontxnalToTxnal.size())));
	arg.push_back(ConstantInt::get(M->getContext(),
	        APInt(32, nontxnalToTxnal.begin()->second.size())));
	CallInst* r = CallInst::Create(tif->getFunction(
	        Interface::tanger_stm_indirect_init_multi),
	        arg.begin(), arg.end(), "", initInstr);
	tif->setCallProps(r);
	for (std::map<Function*, std::vector<Function*> >::iterator
	        i = nontxnalToTxnal.begin(), ie = nontxnalToTxnal.end();
	        i != ie; i++) {
	    for (size_t j = 0, jE = i->second.size(); j < jE; ++j) {
	        std::vector<Value*> v;
	        //errs() << "Function:" << i->first->getName() << ": " << j << "\n";
	        v.push_back(new BitCastInst(i->first, voidptr, "", initInstr));
	        assert(i->second[j] && "Txnal function not created");
	        v.push_back(new BitCastInst(i->second[j], voidptr,
	                "", initInstr));
	        v.push_back(ConstantInt::get(M->getContext(), APInt(32, j)));
	        r = CallInst::Create(tif->getFunction(
	                Interface::tanger_stm_indirect_register_multi),
	                v.begin(), v.end(), "", initInstr);
	        tif->setCallProps(r);
	    }
	}
}

#ifdef ENABLE_REPORTING
static GlobalVariable* insertGVarStr(Module* M, const char* str)
{
    Type* type = ArrayType::get(IntegerType::get(M->getContext(), 8), strlen(str) + 1);
    GlobalVariable* var = new GlobalVariable(*M, type,
            true, GlobalValue::PrivateLinkage, 0, ".str");
    var->setAlignment(1);
    var->setInitializer(ConstantArray::get(M->getContext(), str, true));
    return var;
}
#endif

/**
 * Creates static ctors/dtors that call init/shutdown for the STM.
 */
void Transformations::insertSTMSetup(const char* passName)
{
    // create constructor function
    FunctionType* const_func_type = FunctionType::get(Type::getVoidTy(
            M->getContext()), std::vector<const Type*>(), false);
	Function * func_stm_constructor = tif->getOrCreateFunction(
            tif_tanger_stm_constructor, const_func_type);
    BasicBlock * consb1 = BasicBlock::Create(M->getContext(), "entry",
            func_stm_constructor, 0);
    ReturnInst::Create(M->getContext(), consb1);
    CallInst* i = CallInst::Create(
            tif->getFunction(Interface::itm_initProcess), "",
            consb1->getFirstNonPHI());
    tif->setCallProps(i);

#ifdef ENABLE_REPORTING
    // if reporting is enabled, report build info
    PointerType* voidptrty = PointerType::get(IntegerType::get(M->getContext(), 8), 0);
    const Type* voidty = Type::getVoidTy(M->getContext());
    const Type* i32ty = IntegerType::get(M->getContext(), 32);
    Function* func_rs = tif->getOrCreateFunction("tanger_stm_report_start", voidptrty, voidptrty, (Type*)0);
    Function* func_ras = tif->getOrCreateFunction("tanger_stm_report_append_string", voidty, voidptrty, voidptrty, voidptrty, (Type*)0);
    Function* func_rai = tif->getOrCreateFunction("tanger_stm_report_append_int", voidty, voidptrty, voidptrty, i32ty, (Type*)0);
    Function* func_rab = tif->getOrCreateFunction("tanger_stm_report_append_bool", voidty, voidptrty, voidptrty, i32ty, (Type*)0);
    Function* func_rf = tif->getOrCreateFunction("tanger_stm_report_finish", voidty, voidptrty, voidptrty, (Type*)0);
    // void* rh = tanger_stm_report_start("DTMC");
    IRBuilder<> irb(consb1, consb1->getTerminator());
    Value* strdtmc = irb.CreateConstInBoundsGEP2_32(insertGVarStr(M, "DTMC"), 0, 0);
    CallInst* rs = irb.CreateCall(func_rs, strdtmc, "rh");
    // LLVM pass name
    irb.CreateCall3(func_ras, rs,
            irb.CreateConstInBoundsGEP2_32(insertGVarStr(M, "passname"), 0, 0),
            irb.CreateConstInBoundsGEP2_32(insertGVarStr(M, passName), 0, 0));
    // tanger_stm_report_append_string(rh, "svnurl", BUILDINFO_SVN_URL);
    irb.CreateCall3(func_ras, rs,
            irb.CreateConstInBoundsGEP2_32(insertGVarStr(M, "svnurl"), 0, 0),
            irb.CreateConstInBoundsGEP2_32(insertGVarStr(M, BUILDINFO_SVN_URL), 0, 0));
    // tanger_stm_report_append_int(rh, "svnmaxrev", BUILDINFO_SVN_MAXREV);
    irb.CreateCall3(func_rai, rs,
            irb.CreateConstInBoundsGEP2_32(insertGVarStr(M, "svnmaxrev"), 0, 0),
            ConstantInt::get(i32ty, BUILDINFO_SVN_MAXREV, true));
    // tanger_stm_report_append_bool(rh, "svnwcmodified", BUILDINFO_SVN_CHANGED);
    irb.CreateCall3(func_rab, rs,
            irb.CreateConstInBoundsGEP2_32(insertGVarStr(M, "svnwcmodified"), 0, 0),
            ConstantInt::get(i32ty, BUILDINFO_SVN_CHANGED, true));
    // tanger_stm_report_finish(rh, "DTMC");
    irb.CreateCall2(func_rf, rs, strdtmc);
#ifdef BUILDINFO_DSA_SVN_URL
    // Report DSA SVN info
    Value* strdsa = irb.CreateConstInBoundsGEP2_32(insertGVarStr(M, "DSA"), 0, 0);
    rs = irb.CreateCall(func_rs, strdsa, "rh");
    irb.CreateCall3(func_ras, rs,
            irb.CreateConstInBoundsGEP2_32(insertGVarStr(M, "svnurl"), 0, 0),
            irb.CreateConstInBoundsGEP2_32(insertGVarStr(M, BUILDINFO_DSA_SVN_URL), 0, 0));
    irb.CreateCall3(func_rai, rs,
            irb.CreateConstInBoundsGEP2_32(insertGVarStr(M, "svnmaxrev"), 0, 0),
            ConstantInt::get(i32ty, BUILDINFO_DSA_SVN_MAXREV, true));
    irb.CreateCall3(func_rab, rs,
            irb.CreateConstInBoundsGEP2_32(insertGVarStr(M, "svnwcmodified"), 0, 0),
            ConstantInt::get(i32ty, BUILDINFO_DSA_SVN_CHANGED, true));
    irb.CreateCall2(func_rf, rs, strdsa);
#endif
#endif

	std::vector<const Type*> StructTy_2_fields;
    StructTy_2_fields.push_back(IntegerType::get(M->getContext(), 32));
    std::vector<const Type*> FuncTy_4_args;

	FunctionType* FuncTy_4 = FunctionType::get(
            Type::getVoidTy(M->getContext()), FuncTy_4_args, false);

	PointerType* PointerTy_3 = PointerType::get(FuncTy_4, 0);
    StructTy_2_fields.push_back(PointerTy_3);
    StructType* StructTy_2 = StructType::get(M->getContext(),
            StructTy_2_fields, false);
    // XXX: this is probably supposed to be the priority, but current LLVM
    // ignores this number. Fix priority handling so that STM is initialized
    // first?
    ConstantInt* const_int32_12 = ConstantInt::get(M->getContext(), APInt(32,
            65535));

	GlobalVariable * global_ctors = M->getGlobalVariable("llvm.global_ctors");

    std::vector<Constant*> initElems;
    if (global_ctors) {
        // get other initializers
        ConstantArray* init = cast<ConstantArray>(global_ctors->getInitializer());
        for (unsigned i = 0; i < init->getNumOperands(); i++)
            initElems.push_back(init->getOperand(i));
        global_ctors->eraseFromParent();
    }

    // add TM initializer
    std::vector<Constant*> const_struct_11_fields;
    const_struct_11_fields.push_back(const_int32_12);
    const_struct_11_fields.push_back(func_stm_constructor);
    Constant* const_struct_11 = ConstantStruct::get(StructTy_2,
            const_struct_11_fields);
    initElems.push_back(const_struct_11);

    // create new global ctors
    ArrayType* ArrayTy_1 = ArrayType::get(StructTy_2, initElems.size());
    Constant* newInit = ConstantArray::get(ArrayTy_1, initElems);
    global_ctors = new GlobalVariable(*M, ArrayTy_1, false,
            GlobalValue::AppendingLinkage, newInit,
            "llvm.global_ctors");

	if (clAddCallToTMShutdown) {
	    // create destructor function
	    Function * func_stm_destructor = tif->getOrCreateFunction(
	            tif_tanger_stm_destructor, const_func_type);
	    BasicBlock * desb1 = BasicBlock::Create(M->getContext(), "entry",
	            func_stm_destructor, 0);
	    ReturnInst::Create(M->getContext(), desb1);
	    i = CallInst::Create(tif->getFunction(Interface::itm_finalizeProcess), "",
	            desb1->getFirstNonPHI());
	    tif->setCallProps(i);

	    // now handle dtors
	    GlobalVariable * global_dtors = M->getGlobalVariable("llvm.global_dtors");
	    if (!global_dtors) {
	        ArrayType* ArrayTy_1 = ArrayType::get(StructTy_2, 1);

	        GlobalVariable* global_dtors = new GlobalVariable(*M, ArrayTy_1, false,
	                GlobalValue::AppendingLinkage, 0, // has initializer, specified below
	                "llvm.global_dtors");

	        std::vector<Constant*> const_array_13_elems;
	        std::vector<Constant*> const_struct_14_fields;
	        const_struct_14_fields.push_back(const_int32_12);
	        const_struct_14_fields.push_back(func_stm_destructor);
	        Constant* const_struct_14 = ConstantStruct::get(StructTy_2,
	                const_struct_14_fields);
	        const_array_13_elems.push_back(const_struct_14);
	        Constant* const_array_13 = ConstantArray::get(ArrayTy_1,
	                const_array_13_elems);

	        global_dtors->setInitializer(const_array_13);
	    }
	    else assert(0 && "adding to global dtors not supported");
	}
}


/**
 * Initializes a load/store transformation.
 * Basically, it gets or creates the required load/store functions.
 */
void TransformLoadStore::init(Interface* tif, const char* suffix)
{
    this->tif = tif;
    if (!suffix) suffix = "";
    initLoadStore(tif, 0, suffix);
}


TransformLoadStore::~TransformLoadStore()
{
    delete[] func_load;
    delete[] func_store;
}


/**
 * Get or create load/store functions.
 * Does not create special functions to load/store from/to memory regions.
 */
void TransformLoadStore::initLoadStore(Interface* tif,
        std::vector<const Type*>* additionalArgs, const char* nameModifier)
{
    const static unsigned loadstore_funcs_count_old = 7;
    const static unsigned loadstore_funcs_count_ABI = 4;
    const Type* type_voidptr = tif->getTypeVoidPtr();
    unsigned loadstore_funcs_count = clABILSFuncs ?
            loadstore_funcs_count_ABI : loadstore_funcs_count_old;

    int bitsABI[loadstore_funcs_count_ABI] = { 8, 16, 32, 64 };
    static const char* bitstrABI[loadstore_funcs_count_ABI] =
        { "U1", "U2", "U4", "U8" };
    int bitsOld[loadstore_funcs_count_old] = { 8, 16, 16, 32, 32, 64, 64 };
    static const char* bitstrOld[loadstore_funcs_count_old] = { "8", "16",
            "16aligned", "32", "32aligned", "64", "64aligned" };
    int *bits = clABILSFuncs ? bitsABI : bitsOld;
    const char **bitstr = clABILSFuncs ? bitstrABI : bitstrOld;

    func_load = new Function*[loadstore_funcs_count];
    func_store = new Function*[loadstore_funcs_count];
    for (unsigned i = 0; i < loadstore_funcs_count; i++) {
        const IntegerType* intty = IntegerType::get(
                tif->getModule()->getContext(), bits[i]);
        PointerType* intptrty = PointerType::getUnqual(intty);
        std::vector<const Type*> fargs;
        if (!clABILSFuncs) fargs.push_back(type_voidptr); // txn desc
        if (additionalArgs) fargs.insert(fargs.end(),
                additionalArgs->begin(), additionalArgs->end());
        fargs.push_back(intptrty);
        const FunctionType* ft =
                FunctionType::get(intty, fargs, false);
        std::string name = clABILSFuncs? tif_itm_load : tif_tanger_stm_load;
        if (clABILSFuncs) {
            name += bitstr[i];
            name += nameModifier;
        }
        else {
            name += nameModifier;
            name += bitstr[i];
        }
        func_load[i] = tif->getOrCreateFunction(name.c_str(), ft);
        if (clAlwaysInlineLSFuncs) func_load[i]->addFnAttr(
                Attribute::AlwaysInline);
    }
    for (unsigned i = 0; i < loadstore_funcs_count; i++) {
        const IntegerType* intty = IntegerType::get(
                tif->getModule()->getContext(), bits[i]);
        PointerType* intptrty = PointerType::getUnqual(intty);
        std::vector<const Type*> fargs;
        if (!clABILSFuncs) fargs.push_back(type_voidptr); // txn desc
        if (additionalArgs) fargs.insert(fargs.end(),
                additionalArgs->begin(), additionalArgs->end());
        fargs.push_back(intptrty);
        fargs.push_back(intty);
        const FunctionType* ft = FunctionType::get(Type::getVoidTy(
                tif->getModule()->getContext()), fargs, false);
        std::string name = clABILSFuncs? tif_itm_store : tif_tanger_stm_store;
        if (clABILSFuncs) {
            name += bitstr[i];
            name += nameModifier;
        }
        else {
            name += nameModifier;
            name += bitstr[i];
        }
        func_store[i] = tif->getOrCreateFunction(name.c_str(), ft);
        if (clAlwaysInlineLSFuncs) func_store[i]->addFnAttr(
                Attribute::AlwaysInline);
    }
}


/**
 * Returns the load/store function for a certain type, or zero if there
 * is no suitable function.
 */
Function* TransformLoadStore::getIntLoadStoreFunction(Function** functions, const Type* type, bool aligned)
{
    const IntegerType *ti = cast<IntegerType>(type);
    Function* f = 0;
    if (clABILSFuncs) {
        switch (ti->getBitWidth()) {
        case 8: f = functions[0]; break;
        case 16: f = functions[1]; break;
        case 32: f = functions[2]; break;
        case 64: f = functions[3]; break;
        }
    }
    else {
        switch (ti->getBitWidth()) {
        case 8: f = functions[0]; break;
        case 16: f = functions[aligned ? 2 : 1]; break;
        case 32: f = functions[aligned ? 4 : 3]; break;
        case 64: f = functions[aligned ? 6 : 5]; break;
        }
    }
    return f;
}


/**
 * Returns the type that the provided type must be casted to before a
 * load or store.
 */
const Type* TransformLoadStore::getLoadStoreConversion(Module* M,
        TargetData* TD, const Type* type)
{
    const Type* to = type;
    if (isa<PointerType> (type)) {
        // convert to integer
        if (M->getPointerSize() == Module::Pointer32)
            to = IntegerType::get(M->getContext(), 32);
        else if (M->getPointerSize() == Module::Pointer64)
            to = IntegerType::get(M->getContext(), 64);
        else assert(0 && "unsupported pointer size");
    }
    else if (type->isFloatingPointTy()) {
        // convert to integer
        if (type == Type::getFloatTy(M->getContext()))
            to = IntegerType::get(M->getContext(), 32);
        else if (type == Type::getDoubleTy(M->getContext()))
            to = IntegerType::get(M->getContext(), 64);
    }
    else if (type->isIntegerTy() && cast<IntegerType> (type)->getBitWidth() < 8) {
        // use a bigger integer type if we are allowed to store more
        // TODO: use this for bigger integers too?
        to = IntegerType::get(M->getContext(), TD->getTypeStoreSizeInBits(type));
    }
    return to;
}


/**
 * Replaces a single load instruction with a matching call to a load function.
 */
Value* TransformLoadStore::replaceLoad(LoadInst* li, Value* tx, std::vector<
        Value*>* additionalArgs)
{
    const Type* type = li->getType();
    const Type* castType = type;

    type = getLoadStoreConversion(tif->getModule(), tif->getTargetData(), type);

    if (isa<VectorType>(type)) {
        // TODO implement load/store for vectors
        assert(0 && "vector types are not yet supported");
    }

    if (type->isIntegerTy()) {
        unsigned bitwidth = cast<IntegerType> (type)->getBitWidth();
        bool aligned = (li->getAlignment() * 8 >= bitwidth);
        Function* f = getIntLoadStoreFunction(func_load, type,
                aligned && clAlignedLSFuncs);
        if (f) {
            if (type == castType && bitwidth < 8) {
                assert(bitwidth == 1 && "type is smaller than i8, but is not i1");
                type = IntegerType::get(tif->getModule()->getContext(), 8);
            }
            if (type != castType) {
                Value* from = new BitCastInst(li->getPointerOperand(),
                        PointerType::getUnqual(type), "", li);
                std::vector<Value*> v;
                if (!clABILSFuncs) v.push_back(tx);
                if (additionalArgs) v.insert(v.end(),
                        additionalArgs->begin(), additionalArgs->end());
                v.push_back(from);
                Instruction* ci = CallInst::Create(f, v.begin(), v.end(),
                        li->getName(), li);
                tif->setCallProps(cast<CallInst> (ci));
                // pointers must be casted using inttoptr/ptrtoint
                // if we need less bits for integer types, we truncate
                if (isa<PointerType> (castType))
                    ci = new IntToPtrInst(ci, castType);
                else if (isa<IntegerType> (castType))
                    ci = new TruncInst(ci, castType);
                else ci = new BitCastInst(ci, castType);
                ReplaceInstWithInst(li, ci);
                return ci;
            }
            else {
                std::vector<Value*> v;
                if (!clABILSFuncs) v.push_back(tx);
                if (additionalArgs) v.insert(v.end(),
                        additionalArgs->begin(), additionalArgs->end());
                v.push_back(li->getPointerOperand());
                CallInst* ci = CallInst::Create(f, v.begin(), v.end(),
                        li->getName());
                tif->setCallProps(ci);
                ReplaceInstWithInst(li, ci);
                return ci;
            }
        }
    }
    errs() << "ERROR: load to unsupported type ";
    type->print(errs());
    errs() << "\n";
    fatalError();
    return li; // nothing changed
}


/**
 * Replaces a single store instruction with a matching call to a store function.
 */
void TransformLoadStore::replaceStore(StoreInst* si, Value* tx, std::vector<
        Value*>* additionalArgs)
{
    // operands: store(value, ptr)
    Value* value = si->getOperand(0);
    const Type* type = value->getType();
    const Type* castType = type;

    type = getLoadStoreConversion(tif->getModule(), tif->getTargetData(), type);

    if (isa<VectorType>(type)) {
        // TODO implement load/store for vectors
        assert(0 && "vector types are not yet supported");
    }

    if (type->isIntegerTy()) {
        unsigned bitwidth = cast<IntegerType> (type)->getBitWidth();
        bool aligned = (si->getAlignment() * 8 >= bitwidth);
        Function* f = getIntLoadStoreFunction(func_store, type,
                aligned && clAlignedLSFuncs);
        if (f) {
            if (type == castType && bitwidth < 8) {
                assert(bitwidth == 1 && "type is smaller than i8, but is not i1");
                type = IntegerType::get(tif->getModule()->getContext(), 8);
            }
            Value *target = si->getPointerOperand();
            if (type != castType) {
                target = new BitCastInst(target, PointerType::getUnqual(type),"", si);
                // if the function takes more bits than available, we zero extend to the larger type
                if (isa<PointerType> (castType))
                    value = new PtrToIntInst(value, type, "", si);
                else if (isa<IntegerType> (castType))
                    value = new ZExtInst(value, type, "", si);
                else value = new BitCastInst(value, type, "", si);
            }
            std::vector<Value*> v;
            if (!clABILSFuncs) v.push_back(tx);
            if (additionalArgs) v.insert(v.end(),
                    additionalArgs->begin(), additionalArgs->end());
            v.push_back(target);
            v.push_back(value);
            CallInst* ci = CallInst::Create(f, v.begin(), v.end(),
                    si->getName());
            tif->setCallProps(ci);
            ReplaceInstWithInst(si, ci);
            return;
        }
    }
    errs() << "ERROR: store to unsupported type ";
    type->print(errs());
    errs() << "\n";
    fatalError();
}


/**
 * Replaces memcpy/memmove/memset with a call to the matching ABI function.
 */
void TransformLoadStore::replaceMemIntrinsic(MemIntrinsic* mi, Value* tx, std::vector<
        Value*>* additionalArgs)
{
    // Casts for destination and length.
    Value* dest = mi->getRawDest();
    if (dest->getType() != tif->getTypeVoidPtr())
        dest = new BitCastInst(dest, tif->getTypeVoidPtr(), "", mi);
    Value* length = mi->getLength();
    const IntegerType* lengthType = cast<IntegerType>(length->getType());
    unsigned sizetWidth = cast<IntegerType>(tif->getTypeSizet())->getBitWidth();
    if (lengthType->getBitWidth() > sizetWidth)
        length = new TruncInst(length, tif->getTypeSizet(), "", mi);
    if (lengthType->getBitWidth() < sizetWidth)
        length = new ZExtInst(length, tif->getTypeSizet(), "", mi);

    // Create replacement call.
    CallInst *ci;
    if (MemSetInst *msi = dyn_cast<MemSetInst>(mi)) {
        Value* value = new ZExtInst(msi->getValue(),
                IntegerType::getInt32Ty(mi->getContext()), "", mi);
        std::vector<Value*> v;
        v.push_back(dest);
        v.push_back(value);
        v.push_back(length);
        if (additionalArgs) v.insert(v.end(),
                additionalArgs->begin(), additionalArgs->end());
        ci = CallInst::Create(tif->getFunction(Interface::itm_memsetwt),
                v.begin(), v.end(), mi->getName());
    }
    else {
        assert(isa<MemMoveInst>(mi) || isa<MemCpyInst>(mi));
        MemTransferInst *mti = cast<MemTransferInst>(mi);
        Value* src = mti->getRawSource();
        if (src->getType() != tif->getTypeVoidPtr())
            src = new BitCastInst(src, tif->getTypeVoidPtr(), "", mi);
        std::vector<Value*> v;
        v.push_back(dest);
        v.push_back(src);
        v.push_back(length);
        if (additionalArgs) v.insert(v.end(),
                additionalArgs->begin(), additionalArgs->end());
        ci = CallInst::Create(tif->getFunction(
                isa<MemMoveInst>(mi) ? Interface::itm_memmovertwt : Interface::itm_memcpyrtwt),
                v.begin(), v.end(), mi->getName());
    }
    tif->setCallProps(ci);
    ReplaceInstWithInst(mi, ci);
}


DefaultTransformer::DefaultTransformer(Interface* tif,
        FuncInfoProvider* fip, uint32_t abiID, uint32_t internalID,
        const std::string& suffix)
: Transformer(abiID, internalID), tif(tif), suffix(suffix), fip(fip)
{
    ls.init(tif, suffix.c_str());
}

void DefaultTransformer::transformCodePath(BBSet& bbs,
        const InstrSet& doNotTransform, Value* txnDesc,
        ValueMap<const Value*, Value*>* origToClone)
{
    for (BBSet::iterator i = bbs.begin(), ie = bbs.end(); i != ie; i++)
        transformBB(*i, doNotTransform, txnDesc, origToClone);
}


/** Transforms a single basic block.
 * Does not transform instructions that are contained in doNotTransform.
 * If origToClone is nonzero, then txnDesc and doNotTransform
 * point to the original values, but we actually transform the
 * clone that is mapped in origToClone instead. The clone is expected
 * to be a perfect clone (e.g., no instructions have been swapped since
 * cloning the BB).
 */
void DefaultTransformer::transformBB(BasicBlock* bb,
        const InstrSet& doNotTransform, Value* txnDesc,
        ValueMap<const Value*, Value*>* origToClone)
{
    BasicBlock* origBB = bb;
    if (origToClone) {
        // Make sure we transform the clone and get properties from the original BB.
        bb = cast<BasicBlock> ((*origToClone)[bb]);
        assert(bb && "BB not found in origToClone mapping");
        txnDesc = (*origToClone)[txnDesc];
        assert(txnDesc && "txnDesc not found in origToClone mapping");
    }

    // iterate over instructions. The BB should initially be a perfect clone
    // of the original BB, so we can just iterate over both and don't need to
    // look up the cloned instruction using origToClone all the time.
    for (BasicBlock::iterator i = bb->begin(), ie = bb->end(),
            iorig = origBB->begin(), inext, iorignext;
            i != ie; i = inext, iorig = iorignext) {
        // Get next instruction before modifying the current instruction (it's an ilist...)
        inext = i;
        inext++;
        iorignext = iorig;
        iorignext++;

        // Skip instructions that we don't need to transform.
        if (doNotTransform.count(iorig)) {
            stat_instrsNotTransformed++;
            continue;
        }

        // load/store
        if (LoadInst *li = dyn_cast<LoadInst>(&*i)) {
            if (li->isVolatile()) {
                // loading from a volatile isn't common in sequential code,
                // but in concurrent code...
                printWarning(*bb->getParent(), *bb)
                        << "load from volatile memory location\n";
            }
            transformLoad(li, txnDesc);
            stat_loads++;
        }
        else if (StoreInst *si = dyn_cast<StoreInst>(&*i)) {
            if (si->isVolatile()) {
                // storing to a volatile isn't common in sequential code,
                // but in concurrent code...
                printWarning(*bb->getParent(), *bb)
                        << "store to volatile memory location\n";
            }
            // only store if the target is not an allocated exception
            transformStore(si, txnDesc);
            stat_stores++;
        }
        else if (MemIntrinsic *mi = dyn_cast<MemIntrinsic>(&*i))
            transformMemIntrinsic(mi, txnDesc);

        // calls and invokes
        else if (isa<CallInst>(&*i) || isa<InvokeInst>(&*i)) {
            CallSite cs = CallSite::get(&*i);
            Function *f = cs.getCalledFunction();
            // ignore bitcasts and constant expressions
            if (!f)
                f = dyn_cast<Function>(cs.getCalledValue()->getUnderlyingObject());
            if (!f) {
                // indirect call
                // we don't seem to have to do anything special for invokes
                //if (isa<InvokeInst>(&*i))
                //    printWarning(*bb->getParent()) << " indirect invoke not tested\n";
                Value *newTarget = Transformations::getIndirectCallTarget(
                        cs, tif, getInternalID(), getABIID());
                cs.setCalledFunction(newTarget);
                ++stat_resolvedIndirectCalls;
            }
            // skip debug intrinsics
            else if (f->isIntrinsic() && isa<DbgInfoIntrinsic>(cs.getInstruction())) {}
            // TODO replace this hack with the proper use use _ITM_registerThrownObject
//            else if (tif.isExceptionHandling(f)) {
//                if(f->getName().find("__cxa_allocate_exception") == 0) {
//                    for (Value::use_iterator i = ci->use_begin(), e = ci->use_end(); i != e; ++i)
//                      if (BitCastInst *Inst = dyn_cast<BitCastInst>(*i)) {
//                          for (Value::use_iterator ii = Inst->use_begin(), ie = Inst->use_end(); ii != ie; ++ii)
//                          allocatedExceptions.insert(*ii);
//                      }
//                }
//            }
            else {
                FuncInfo *targetFI = fip->getOrCreateFuncInfo(f);
                assert(targetFI->type == FuncInfo::Callable || targetFI->type == FuncInfo::Pure);
                // otherVersion contains either same function (if pure) or txnal version
                if (targetFI->type == FuncInfo::Callable
                        && !targetFI->checkResult == FuncInfo::CompletelyValid) {
                    // If no txnal can be created, then
                    // clResolveNontxnalDirectCalls must be true because the
                    // current function wouldn't be valid for transformations
                    // otherwise.
                    // Therefore, call the resolve library function, which will
                    // probably switch to serial mode at runtime.
                    DEBUG(errs() << "  redirecting call to "<<f->getName()<<"(...) to resolver\n");
                    Value *newTarget = Transformations::getIndirectCallTarget(
                            cs, tif, getInternalID(), getABIID());
                    cs.setCalledFunction(newTarget);
                    stat_resolvedDirectCalls++;
                }
                else {
                    //DEBUG(errs() << "  redirecting call to "<<f->getName()<<"(...) to txnal version\n");
                    cs.setCalledFunction(targetFI->getTxnalFunction(
                            getInternalID()));
                    stat_redirectedCalls++;
                }
            }
        }
    }
}


Value* DefaultTransformer::transformLoad(LoadInst* li, Value* txnDesc)
{
    return ls.replaceLoad(li, txnDesc, 0);

}


void DefaultTransformer::transformStore(StoreInst* si, Value* txnDesc)
{
    ls.replaceStore(si, txnDesc, 0);
}

void DefaultTransformer::transformMemIntrinsic(MemIntrinsic* mi, Value* txnDesc)
{
    ls.replaceMemIntrinsic(mi, txnDesc, 0);
}
