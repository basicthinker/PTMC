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
 * TM interfaces.
 */
#ifndef TANGER_PASS_INTERFACE_H_
#define TANGER_PASS_INTERFACE_H_

#include <stdarg.h>
#include <vector>

#include "llvm/Instructions.h"
#include "llvm/IntrinsicInst.h"
#include "llvm/Function.h"
#include "llvm/Module.h"
#include "llvm/Type.h"
#include "llvm/CallingConv.h"
#include "llvm/Target/TargetData.h"

using namespace llvm;

namespace tanger {

class Interface {
public:
	enum Functions {
		// Application-level interface
		tanger_begin,
		tanger_commit,

		// ITM ABI
		itm_begin,
		itm_commit,
		itm_abort,
		itm_trycommit,
		itm_initProcess,
		itm_finalizeProcess,
		itm_memcpyrtwt,
		itm_memmovertwt,
		itm_memsetwt,

		// Old STM ABI
		tanger_stm_get_tx,
		tanger_stm_indirect_resolve_multi,
		tanger_stm_indirect_init_multi,
		tanger_stm_indirect_register_multi,

		// functions that have default wrappers
		itm_malloc,
		malloc,
		itm_calloc,
		calloc,
		tanger_stm_realloc,
		realloc,
		itm_free,
		free,
		// functions that are tm_pure by default
		assert_fail,
		// End Marker
		InterfaceFunctionsCount
	};
	void init(Module* M, TargetData* TD);
	void registerFunction(Functions id, const char* name, const Type* rettype, ...);
	void registerFunction(Functions id, bool varArg, const char* name, const Type* rettype, ...);
	void registerFunction(Functions id, const char* name, const FunctionType* type);
	Module* getModule() { return M; }
	TargetData* getTargetData() { return TD; }
	Function* getFunction(Functions id);
	Function* getOrCreateFunction(const char *name, const FunctionType* type);
	Function* getOrCreateFunction(const char *name, const Type* rettype, ...);
	Function* getIntrinsic(Intrinsic::ID id);
	const PointerType* getTypeVoidPtr();
	const IntegerType* getTypeSizet();
	bool isBegin(const Function* func);
	bool isCommit(const Function* func);
	void setCallProps(CallInst* ci);
private:
	CallingConv::ID getCallingConv();
protected:
	Module* M;
	TargetData* TD;
	std::vector<Function*> functions;
};

// Aliases
extern const char* tif_tanger_prefix;
extern const char* tif_stminternal_prefix;
extern const char* tif_txnalwrapper_prefix;
extern const char* tif_txnalwrapperpure_prefix;
extern const char* tif_txnalwrapperintrinsic_prefix;
// Annotations
extern const char* tif_txn_pure;
extern const char* tif_txn_callable;
extern const char* tif_txn_wrapper;
extern const char* tif_txn_noinline;
extern const char* tif_txn_alwaysinline;

// Internal STM interface
extern const char* tif_tanger_stm_save_restore_stack;
extern const char* tif_tanger_stm_load;
extern const char* tif_tanger_stm_store;
extern const char* tif_itm_load;
extern const char* tif_itm_store;
extern const char* tif_tanger_stm_constructor;
extern const char* tif_tanger_stm_destructor;

}

#endif /*TANGER_PASS_INTERFACE_H_*/
