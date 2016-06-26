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
#ifndef TANGER_PASS_TRANSFORM_H_
#define TANGER_PASS_TRANSFORM_H_

#include <map>
#include <set>

#include "llvm/Instructions.h"
#include "llvm/IntrinsicInst.h"
#include "llvm/BasicBlock.h"
#include "llvm/Module.h"
#include "llvm/ADT/ValueMap.h"
#include "llvm/Support/CallSite.h"
#include "interface.h"
#include "funcinfo.h"

using namespace llvm;

namespace tanger {

class Transformations {
protected:
    Module* M;
    TargetData* TD;
    Interface* tif;

    Function* func_get_tx;
    Function* func_save_restore_stack;
    Function* func_begin;
    Function* func_commit;

public:
    ~Transformations();
    bool init(Interface* tif);
    Value* insertGetTx(Instruction* insertBefore);
    CallInst* replaceBegin(CallInst* ci);
    CallInst* replaceCommit(CallInst* ci);
    void replaceWithNoop(Instruction* i);
    Value* insertGetABICodePathID(Value* beginRetVal,
            Instruction* insertBefore);
    void removeCalls(Function* f);
    void insertIndirectCallInit(Instruction* initInstr, std::map<Function*,
            std::vector<Function*> > nontxnalToTxnal);
    static Value* getIndirectCallTarget(CallSite& cs, Interface* tif,
            unsigned internalID, uint32_t abiID);
    void insertSTMSetup(const char* passName);

    static void getCalls(Constant* constant, std::vector<CallInst*>& calls);
};


/**
 * Auxiliary class for the transformation of loads and stores.
 */
class TransformLoadStore {
public:
protected:
    Interface* tif;
    Function** func_load;       // 8b, 16b, 16baligned, 32b, 32b aligned, 64b, 64b aligned or U1, U2, U4, U8
    Function** func_store;      // 8b, 16b, 16baligned, 32b, 32b aligned, 64b, 64b aligned or U1, U2, U4, U8
public:
    TransformLoadStore() :
        func_load(0), func_store(0)
    {
    }
    virtual ~TransformLoadStore();
    Value* replaceLoad(LoadInst* li, Value* tx,
            std::vector<Value*>* additionalArgs);
    void replaceStore(StoreInst* si, Value* tx,
            std::vector<Value*>* additionalArgs);
    void replaceMemIntrinsic(MemIntrinsic* mi, Value* tx,
            std::vector<Value*>* additionalArgs);
    void init(Interface* tif, const char* suffix = 0);
protected:
    void initLoadStore(Interface* tif,
            std::vector<const Type*>* additionalArgs, const char* nameModifier);
    Function* getIntLoadStoreFunction(Function** functions, const Type* type,
            bool aligned);
    const Type* getLoadStoreConversion(Module* M, TargetData* TD,
            const Type* type);
};


/**
 * Transformers are kinds of instrumentation of txnal code.
 */
class Transformer {
    uint32_t abiID;
    uint32_t internalID;
public:
    Transformer(uint32_t abiID, uint32_t internalID)
    : abiID(abiID), internalID(internalID) {}
    virtual ~Transformer() {}
    /** Transforms a code path consisting of a set of basic blocks.
     * Does not transform instructions that are contained in doNotTransform.
     * If origToClone is nonzero, then bbs, txnDesc, and doNotTransform all
     * point to the original values, but we actually transform the
     * clone that is mapped in origToClone instead. The clone is expected
     * to be a perfect clone (e.g., no instructions have been swapped since
     * cloning the BBs). Note that origToClone is only used for transactional
     * versions but not to map from the original statements of a transaction
     * to cloned code paths.
     */
    virtual void transformCodePath(BBSet& bbs, const InstrSet& doNotTransform,
            Value* txnDesc, ValueMap<const Value*, Value*>* origToClone = 0) = 0;
    virtual const char* getBBSuffix() { return ""; }
    /// Returns the ABI's ID for this transformation.
    virtual uint32_t getABIID() { return abiID; }
    /// Returns the internal ID for this transformation.
    /// The main transformation pass provides this value.
    virtual uint32_t getInternalID() { return internalID; }
};

/// Doesn't transform anything. Used for sequential code.
class NoChangeTransformer : public Transformer {
public:
    NoChangeTransformer(uint32_t abiID, uint32_t internalID)
    : Transformer(abiID, internalID) {}
    virtual void transformCodePath(BBSet& bbs, const InstrSet& doNotTransform,
            Value* txnDesc, ValueMap<const Value*, Value*>* origToClone = 0) {}
};


/**
 * The default transformation that replaces loads, stores with calls to
 * a set of (custom) load/store TM functions. Calls to functions get
 * redirected to the respective transactional version.
 * TODO return proper ABI ID for this kind of TM instrumentation
 */
class DefaultTransformer : public Transformer {
    Interface* tif;
    std::string suffix;
    TransformLoadStore ls;
    FuncInfoProvider* fip;
public:
    DefaultTransformer(Interface* tif, FuncInfoProvider* fip,
            uint32_t abiID, uint32_t internalID, const std::string& suffix);
    virtual void transformCodePath(BBSet& bbs, const InstrSet& doNotTransform,
            Value* txnDesc, ValueMap<const Value*, Value*>* origToClone = 0);
    virtual const char* getBBSuffix() { return suffix.c_str(); }
protected:
    virtual void transformBB(BasicBlock* bb, const InstrSet& doNotTransform,
            Value* txnDesc, ValueMap<const Value*, Value*>* origToClone = 0);
    virtual Value* transformLoad(LoadInst* li, Value* txnDesc);
    virtual void transformStore(StoreInst* li, Value* txnDesc);
    virtual void transformMemIntrinsic(MemIntrinsic* mi, Value* txnDesc);
};


} // namespace

#endif /*TANGER_PASS_TRANSFORM_H_*/
