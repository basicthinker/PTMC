/* Copyright (C) 2009  Torvald Riegel
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
#ifndef FUNCINFO_H_
#define FUNCINFO_H_

namespace tanger {

typedef std::set<BasicBlock*> BBSet;
typedef std::set<Instruction*> InstrSet;
typedef std::vector<Instruction*> InstrVector;
typedef std::set<Function*> FunctionSet;
class Transaction;
class Transformer;
typedef std::set<Transaction*> TransactionSet;


/**
 * A static transaction in the application code.
 */
class Transaction {
public:
    class CodePath {
    public:
        BBSet bbs;
        Transformer* transformer;
    };
    typedef std::set<CodePath*> CodePathSet;
public:
    CodePathSet codePaths;
    InstrVector beginCalls;
    InstrVector commitCalls;
    BBSet code;
    /// True if the txn boundaries are declared with the old marker calls.
    bool markerCalls;
    Transaction* parent;
    /// the nesting level, starts at 0 for the outermost txn (assuming that
    /// the function is not txnal). Used to check nesting cycles.
    unsigned level;
    // TODO model "or/else" blocks
public:
    Transaction(bool markerCalls, Transaction* parent)
    : markerCalls(markerCalls), parent(parent), level(parent? parent->level + 1 : 0) {}
    virtual ~Transaction() {}
    /// Merges the other txn's code/begins/commits into this txn. Does not
    /// merge other information (e.g., parent txn).
    void merge(Transaction* other) {
        assert(other->codePaths.empty() && codePaths.empty());
        code.insert(other->code.begin(), other->code.end());
        beginCalls.insert(beginCalls.end(), other->beginCalls.begin(), other->beginCalls.end());
        commitCalls.insert(commitCalls.end(), other->commitCalls.begin(), other->commitCalls.end());
    }
};


/**
 * Used to track Tanger-specific information about functions.
 * The different phases of the transformation (checking, transforming
 * instructions, ...) use this datastructure to communicate.
 */
class FuncInfo {
public:
    /** Callable means that the function is supposed to have txnal version.
     * PotentiallyCallable states that the function could be a target of
     * indirect calls.
     * Pure means that it's safe to call the function even from within txns.
     * TxnalVersion marks txnal versions of functions.
     * Functions without any annotation and that are not potential indirect
     * call targets start in the Unknown state and move to another state once
     * they have been analyzed. Functions in other states will not change their
     * state back to Unknown if they cannot be transformed.
     */
    enum Type { Unknown, Callable, PotentiallyCallable, Pure, TxnalVersion };
    Type type;
    /** Functions can either be completely transformable, or just the code inside
     * of txns started in the function can be transformable, or not transformable at all.
     */
    enum CheckResult { CompletelyValid, TxnsValid, NothingValid, NotYetChecked };
    CheckResult checkResult;
    /// True if the txnal version has been analyzed.
    bool hasBeenChecked;
    /// True if the txnal version has been processed.
    bool txnalVersionProcessed;
    /// Transactions contained in this function.
    TransactionSet txns;
    /// BBs that are not enclosed in transactions
    BBSet levelZeroCode;
    /// The Txn context (e.g, call to tanger_stm_get_tx())
    Value* tx;
    /// A list of begin calls that are not associated with txns yet.
    std::map<BasicBlock*, Instruction*> freeBegins;
    /// A list of commit calls that are not associated with txns yet.
    std::map<BasicBlock*, Instruction*> freeCommits;
private:
    /// The original version of the function (at [0], for clones) or the txnal
    /// versions. For a pure function, this points to the same function the
    /// FuncInfo belongs to.
    std::vector<Function*> otherVersions;
    /// The maximum number of internal IDs for different kinds of
    /// instrumentation used for txnal versions.
    unsigned internalIDMax;
public:
    // A set of functions that will become Callable if this function becomes callable
    FunctionSet levelZeroCallees;

    FuncInfo(unsigned internalIDMax) :
        type(Unknown), checkResult(NotYetChecked), hasBeenChecked(false),
                txnalVersionProcessed(false), tx(0), otherVersions(
                        internalIDMax, (Function*) 0), internalIDMax(
                        internalIDMax)
    {
        assert(internalIDMax > 0 && "Number of versions not set");
    }
    virtual ~FuncInfo()
    {
        for (TransactionSet::iterator i = txns.begin(), ie = txns.end(); i
                != ie; i++)
            delete *i;
    }

    /** Adds a new transaction.
     */
    Transaction* addTxn(Instruction* begin, Transaction* parent,
            bool markerCalls)
    {
        Transaction* txn = new Transaction(markerCalls, parent);
        txn->beginCalls.push_back(begin);
        txns.insert(txn);
        return txn;
    }

    /**
     * Merges one transaction into another and deletes it.
     * This happens when an originally block-scoped transaction statement
     * has two beginTransaction calls that we discover one after the other.
     */
    void mergeAndDeleteTxn(Transaction* target,
            Transaction* toBeMergedandDeleted)
    {
        target->merge(toBeMergedandDeleted);
        TransactionSet::iterator it = txns.find(toBeMergedandDeleted);
        assert(it != txns.end()
                && "merging txn that does not belong to this function");
        txns.erase(it);
        delete toBeMergedandDeleted;
    }

    /** Mark the respective function as tm_pure.
     */
    void setPure(Function* f)
    {
        type = Pure;
        txnalVersionProcessed = true;
        hasBeenChecked = true;
        for (unsigned i = 0; i < internalIDMax; ++i)
            otherVersions[i] = f;
    }

    /** Set the target function as transactional wrapper of this function.
     */
    void setWrapped(Function* target)
    {
        type = FuncInfo::Callable;
        txnalVersionProcessed = true;
        checkResult = FuncInfo::CompletelyValid; // trust the declaration
        hasBeenChecked = true;
        // The wrapper is supposed to cover all instrumentation variants.
        for (unsigned i = 0; i < internalIDMax; ++i)
            otherVersions[i] = target;
    }

    /**
     * This FuncInfo is for a txnal version.
     * Associate this function with the original function and vice versa.
     * If this txnal version is a wrapper function, setForAllIDs must be true
     * (i.e., there exists only one wrapper for all kinds of instrumentation).
     */
    void setTxnalVersion(Function* txnalFunc, unsigned internalID,
            Function* origFunc, FuncInfo* origFI, bool setForAllIDs = false)
    {
        type = TxnalVersion;
        otherVersions[0] = origFunc;
        checkResult = origFI->checkResult;
        hasBeenChecked = true;
        assert(internalID < internalIDMax && "internalID out of bounds.");
        if (!setForAllIDs) origFI->otherVersions[internalID] = txnalFunc;
        else for (unsigned i = 0; i < origFI->internalIDMax; ++i)
            origFI->otherVersions[i] = txnalFunc;

    }

    /** Get the function that is the txnal version of the current function.
     * The internalID is the kind of instrumentation required.
     */
    Function* getTxnalFunction(unsigned internalID)
    {
        assert(internalID < internalIDMax && "internalID out of bounds.");
        return otherVersions[internalID];
    }

    /** Returns all transactional versions.
     */
    std::vector<Function *>& getTxnalFunctions()
    {
        assert(otherVersions.size() == internalIDMax && "Size changed.");
        return otherVersions;
    }

    /// If this is a txnal version, return the original function.
    Function* getOriginalFunction()
    {
        assert(type == TxnalVersion);
        return otherVersions[0];
    }
};


/**
 * Gets or creates FuncInfos.
 */
class FuncInfoProvider {
public:
    virtual FuncInfo* getOrCreateFuncInfo(Function* f) = 0;
};

} // namespace

#endif /* FUNCINFO_H_ */
