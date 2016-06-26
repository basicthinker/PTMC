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
#define DEBUG_TYPE "tanger"

#include <cstdlib>
#include <iostream>
#include "llvm/DerivedTypes.h"
#include "llvm/IntrinsicInst.h"
#include "llvm/CallingConv.h"
#include "llvm/Support/raw_ostream.h"
#include "interface.h"

using namespace tanger;
using namespace llvm;

namespace tanger {

const char* tif_tanger_prefix = "tanger_";
const char* tif_stminternal_prefix = "tanger_stm_";
const char* tif_txnalwrapper_prefix = "tanger_wrapper_";
const char* tif_txnalwrapperpure_prefix ="tanger_wrapperpure_";
const char* tif_txnalwrapperintrinsic_prefix ="tanger.llvm.";

const char* tif_txn_pure = "transaction_pure";
const char* tif_txn_callable = "transaction_callable";
const char* tif_txn_wrapper = "transaction_wrapper";
const char* tif_txn_noinline = "_noinline";
const char* tif_txn_alwaysinline = "_alwaysinline";

// Public interface
const char* tif_tanger_begin = "tanger_begin";
const char* tif_tanger_commit = "tanger_commit";

// Internal STM interface
const char* tif_tanger_stm_get_tx = "tanger_stm_get_tx";
const char* tif_tanger_stm_save_restore_stack = "tanger_stm_save_restore_stack";
const char* tif_tanger_stm_load = "tanger_stm_load";
const char* tif_itm_load = "_ITM_R";
const char* tif_tanger_stm_store = "tanger_stm_store";
const char* tif_itm_store = "_ITM_W";
const char* tif_tanger_stm_constructor = "tanger_stm_constructor";
const char* tif_tanger_stm_destructor = "tanger_stm_destructor";

// generated functions
const char* tif_tanger_stm_indirect_resolve_multi = "tanger_stm_indirect_resolve_multiple";

const char* tif_tanger_stm_indirect_init_multi = "tanger_stm_indirect_init_multiple";

const char* tif_tanger_stm_indirect_register_multi = "tanger_stm_indirect_register_multiple";

}


void Interface::init(Module* M, TargetData* TD)
{
    this->M = M;
    this->TD = TD;
    functions.clear();
    const Type* voidptr = getTypeVoidPtr();
    const Type* voidty = Type::getVoidTy(M->getContext());
    const Type* sizet = getTypeSizet();
    const Type* uint32 = IntegerType::get(M->getContext(), 32);

#define FREG0(ID, RETTYPE) \
	registerFunction(ID, tif_##ID, RETTYPE, NULL);
#define FREG1(ID, RETTYPE, ARG1) \
	registerFunction(ID, tif_##ID, RETTYPE, ARG1, NULL);
#define FREG2(ID, RETTYPE, ARG1, ARG2) \
	registerFunction(ID, tif_##ID, RETTYPE, ARG1, ARG2, NULL);
#define FREG3(ID, RETTYPE, ARG1, ARG2, ARG3) \
	registerFunction(ID, tif_##ID, RETTYPE, ARG1, ARG2, ARG3, NULL);
#define FREGS1(ID, RETTYPE, ARG1) \
	static const char* tif_##ID = # ID; \
	registerFunction(ID, tif_##ID, RETTYPE, ARG1, NULL);
#define FREGS2(ID, RETTYPE, ARG1, ARG2) \
	static const char* tif_##ID = # ID; \
	registerFunction(ID, tif_##ID, RETTYPE, ARG1, ARG2, NULL);

	// Application-level interface
    FREG0(tanger_begin, voidty);
    FREG0(tanger_commit, voidty);

    // malloc et al
    FREGS1(malloc, voidptr, sizet);
    registerFunction(itm_malloc, "_ITM_malloc", voidptr, sizet, NULL);
    FREGS2(calloc, voidptr, sizet, sizet);
    registerFunction(itm_calloc, "_ITM_calloc", voidptr, sizet, sizet, NULL);
    FREGS2(realloc, voidptr, voidptr, sizet);
    FREGS2(tanger_stm_realloc, voidptr, voidptr, sizet);
    FREGS1(free, voidty, voidptr);
    registerFunction(itm_free, "_ITM_free", voidty, voidptr, NULL);

    // ITM interface
    registerFunction(itm_begin, "_ITM_beginTransaction", uint32, uint32,
            NULL);
    registerFunction(itm_commit, "_ITM_commitTransaction", voidty, NULL);
    registerFunction(itm_trycommit, "_ITM_tryCommitTransaction", uint32, NULL);
    registerFunction(itm_abort, "_ITM_abortTransaction", voidty, uint32, NULL);
    registerFunction(itm_initProcess, "_ITM_initializeProcess", uint32, NULL);
    registerFunction(itm_finalizeProcess, "_ITM_finalizeProcess", voidty, NULL);
    registerFunction(itm_memcpyrtwt, "_ITM_memcpyRtWt", voidty, voidptr, voidptr, sizet, NULL);
    registerFunction(itm_memmovertwt, "_ITM_memmoveRtWt", voidty, voidptr, voidptr, sizet, NULL);
    registerFunction(itm_memsetwt, "_ITM_memsetW", voidty, voidptr, uint32, sizet, NULL);

    // old STM ABI
    FREG0(tanger_stm_get_tx, voidptr);

    // old STM ABI: indirect calls
    FREG2(tanger_stm_indirect_resolve_multi, voidptr, voidptr, uint32);
    FREG2(tanger_stm_indirect_init_multi, voidty, uint32, uint32);
    FREG3(tanger_stm_indirect_register_multi, voidty, voidptr, voidptr, uint32);

    // functions allowed in txns
    // TODO: is the 3rd param always unsigned int == 32b?
    static const char* tif_assert_fail = "__assert_fail";
    registerFunction(assert_fail, tif_assert_fail, voidty, voidptr, voidptr,
            IntegerType::get(M->getContext(), 32), voidptr, NULL);

#undef FREG
}


void Interface::registerFunction(Functions id, bool varArg, const char* name,
        const Type* rettype, ...)
{
    // get function type
    va_list ap;
    std::vector<const Type*> args;
    va_start(ap, rettype);
    const Type* t;
    while ((t = va_arg(ap, const Type*)))
        args.push_back(t);
    va_end(ap);
    FunctionType* ft = FunctionType::get(rettype, args, varArg);
    registerFunction(id, name, ft);
}


void Interface::registerFunction(Functions id, const char* name,
        const Type* rettype, ...)
{
    // get function type
    va_list ap;
    std::vector<const Type*> args;
    va_start(ap, rettype);
    const Type* t;
    while ((t = va_arg(ap, const Type*)))
        args.push_back(t);
    va_end(ap);
    FunctionType* ft = FunctionType::get(rettype, args, false);
    registerFunction(id, name, ft);
}


void Interface::registerFunction(Functions id, const char* name,
        const FunctionType* type)
{
    // register function
    if (functions.size() < (unsigned) id + 1) functions.resize(id + 1, 0);
    functions[id] = getOrCreateFunction(name, type);
}


Function* Interface::getFunction(Functions id)
{
    Function* f = functions[id];
    assert(f && "unknown function or function not set");
    return f;
}


Function* Interface::getOrCreateFunction(const char *name,
        const FunctionType* type)
{
    Constant *c = M->getOrInsertFunction(name, type);
    Function* f = dyn_cast<Function> (c);
    if (f) {
        f->setCallingConv(getCallingConv());
        return f;
    }
    errs() << "ERROR: function '" << name
            << "' already defined, but has wrong type: ";
    c->print(errs());
    errs() << "\n  we need type: " << *type << "\n";
    // TODO return meaningful value, somehow
    exit(1);
}


Function* Interface::getOrCreateFunction(const char* name, const Type* rettype,
        ...)
{
    va_list ap;
    std::vector<const Type*> args;
    va_start(ap, rettype);
    const Type* t;
    while ((t = va_arg(ap, const Type*)))
        args.push_back(t);
    va_end(ap);
    FunctionType* ft = FunctionType::get(rettype, args, false);
    return getOrCreateFunction(name, ft);
}

Function* Interface::getIntrinsic(Intrinsic::ID id)
{
    return Intrinsic::getDeclaration(M,id);
}


const PointerType* Interface::getTypeVoidPtr()
{
    return PointerType::getUnqual(IntegerType::get(M->getContext(), 8));
}


const IntegerType* Interface::getTypeSizet()
{
    // TODO check portability (size_t)
    if (M->getPointerSize() == Module::Pointer32) return IntegerType::get(
            M->getContext(), 32);
    else if (M->getPointerSize() == Module::Pointer64) return IntegerType::get(
            M->getContext(), 64);
    assert(0 && "unsupported pointer size");
    return 0;
}


bool Interface::isBegin(const Function* func)
{
    return func == getFunction(tanger_begin) || func == getFunction(itm_begin);
}


bool Interface::isCommit(const Function* func)
{
    return func == getFunction(tanger_commit) || func
            == getFunction(itm_commit) || func == getFunction(itm_trycommit);
}


#define ISFUNC(FUNC, ID) \
bool Interface::is##FUNC(const Function* func)\
{return func == getFunction(ID);}


/**
 * Sets properties of calls to TM interface functions, if necessary.
 */
void Interface::setCallProps(CallInst* ci)
{
    ci->setCallingConv(getCallingConv());
}

CallingConv::ID Interface::getCallingConv()
{
    // XXX update according to calling conv required by the ABI (e.g. for x86)
    return CallingConv::C;
}

// TODO keep this until we replace it with proper TM pure decls
//bool Interface::isExceptionHandling(const Function* f)
//{
//	if(isa<EHSelectorInst>(f)) return true;
//	switch(f->getIntrinsicID()) {
//		case Intrinsic::eh_exception:
//		case Intrinsic::eh_selector_i32:
//		case Intrinsic::eh_selector_i64:
//		case Intrinsic::eh_typeid_for_i32:
//		case Intrinsic::eh_typeid_for_i64:
//			return true;
//		default:;
//	}
//	if (f->getName().find("__cxa_begin_catch") == 0) return true;
//	if (f->getName().find("__cxa_allocate_exception") == 0) return true;
//	if (f->getName().find("__cxa_call_unexpected") == 0) return true;
//	if (f->getName().find("_Unwind_Resume") == 0) return true;
//	if (f->getName().find("__cxa_rethrow") == 0) return true;
//	if (f->getName().find("__cxa_end_catch") == 0) return true;
//	if (f->getName().find("__cxa_throw") == 0) return true;
//
//	if (f->getName().find("__gxx_personality_v") == 0) return true;
//
//	return false;
//}
