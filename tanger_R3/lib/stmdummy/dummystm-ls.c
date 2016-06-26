/* Copyright (C) 2007  Torvald Riegel
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
 * A dummy STM that uses a single lock and prints debug messages if necessary.
 *
 * TODO debug flag and debug printf should go to stmsupport
 */

/*#ifndef __USE_UNIX98
for recursive mutex
#define __USE_UNIX98
#endif
#define __USE_GNU 1*/
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <assert.h>

#include "dummystm.h"

/* debug level */
static const int debugMsgs = 0;

static int threadLocalInitialized = 0;

static pthread_key_t threadLocalKey;

static uintptr_t threadCounter = 1;

static pthread_mutex_t globalLock;

typedef struct {
	uintptr_t threadID;
} stm_tx_t;


#define LOAD(bits, type) \
type tanger_stm_load##bits(tanger_stm_tx_t* tx, type *addr) \
{ \
	if (debugMsgs > 1) printf("txn=%p load addr=%p\n", tx, (void*)addr); \
	return *addr; \
}

LOAD(8, uint8_t)
LOAD(16, uint16_t)
LOAD(32, uint32_t)
LOAD(64, uint64_t)
LOAD(16aligned, uint16_t)
LOAD(32aligned, uint32_t)
LOAD(64aligned, uint64_t)

#define ABILOAD(type, suffix) \
        ABILOAD2(type, , suffix) \
        ABILOAD2(type, aR, suffix) \
        ABILOAD2(type, aW, suffix) \
        ABILOAD2(type, fW, suffix)
#define ABILOAD2(type, mod, suffix) \
type ITM_REGPARM _ITM_R##mod##suffix(const type *addr) \
{ \
    if (debugMsgs > 1) printf("load addr=%p\n", (void*)addr); \
    return *addr; \
}
ABILOAD(uint8_t, U1)
ABILOAD(uint16_t, U2)
ABILOAD(uint32_t, U4)
ABILOAD(uint64_t, U8)


#define STORE(bits, type) \
void tanger_stm_store##bits(tanger_stm_tx_t* tx, type *addr, type value) \
{ \
	if (debugMsgs > 1) printf("txn=%p store addr=%p\n", tx, (void*)addr); \
	*addr = value; \
}

STORE(8, uint8_t)
STORE(16, uint16_t)
STORE(32, uint32_t)
STORE(64, uint64_t)
STORE(16aligned, uint16_t)
STORE(32aligned, uint32_t)
STORE(64aligned, uint64_t)

#define ABISTORE(type, suffix) \
        ABISTORE2(type, , suffix) \
        ABISTORE2(type, aR, suffix) \
        ABISTORE2(type, aW, suffix)
#define ABISTORE2(type, mod, suffix) \
void ITM_REGPARM _ITM_W##mod##suffix(type *addr, type value) \
{ \
    if (debugMsgs > 1) printf("store addr=%p\n", (void*)addr); \
    *addr = value; \
}
ABISTORE(uint8_t, U1)
ABISTORE(uint16_t, U2)
ABISTORE(uint32_t, U4)
ABISTORE(uint64_t, U8)


#define MEMOP(op,rmod,wmod) void ITM_REGPARM _ITM_##op##R##rmod##W##wmod(void* target, const void* source, size_t count) \
{ op(target, source, count); }
#define MEMOPS(op) \
        MEMOP(op,n,t) \
        MEMOP(op,n,taW) \
        MEMOP(op,n,taR) \
        MEMOP(op,t,n) \
        MEMOP(op,taW,n) \
        MEMOP(op,taR,n) \
        MEMOP(op,t,t) \
        MEMOP(op,taW,t) \
        MEMOP(op,taR,t) \
        MEMOP(op,t,taW) \
        MEMOP(op,t,taR)
MEMOPS(memcpy)
MEMOPS(memmove)

#define MEMSET(mod) void ITM_REGPARM _ITM_memset##mod(void* target, int c, size_t count) \
{ memset(target, c, count); }
MEMSET(W)
MEMSET(WaR)
MEMSET(WaW)


/**
 * Starts or restarts a new transaction.
 */
uint32_t ITM_REGPARM _ITM_beginTransaction(uint32_t properties,...)
{
	pthread_mutex_lock(&globalLock);
/*    tanger_stm_tx_t* tx = tanger_stm_get_tx();
	if (debugMsgs) printf("txn=%p start\n", tx);*/
	return 0x02; /* a_runUninstrumentedCode     = 0x02 */
}

/**
 * Tries to commit a transaction.
 * We can ignore longjmp, and we always commit successfully.
 */
void ITM_REGPARM _ITM_commitTransaction()
{
/*    tanger_stm_tx_t* tx = tanger_stm_get_tx();
    if (debugMsgs) printf("txn=%p commit\n", tx);*/
	pthread_mutex_unlock(&globalLock);
}

bool ITM_REGPARM _ITM_tryCommitTransaction()
{
    /* we can always commit */
    return 1;
}


/**
 * Per-thread initialization.
 */
static void tanger_stm_thread_init() __attribute__((noinline));
static void tanger_stm_thread_init()
{
    stm_tx_t* tx;

    pthread_mutex_lock(&globalLock);

    /* set transaction data */
    if ((tx = malloc(sizeof(stm_tx_t))) == NULL) {
        perror("malloc");
        exit(1);
    }
    tx->threadID = threadCounter;
    pthread_setspecific(threadLocalKey, tx);
    threadCounter++;

    pthread_mutex_unlock(&globalLock);
}



/**
 * Returns the calling thread's transaction descriptor.
 * TODO specify when the function is supposed to return a valid value (e.g.,
 * only in transactional code or always)
 */
tanger_stm_tx_t* tanger_stm_get_tx()
{
    tanger_stm_tx_t* tx;
    while (!(tx = (tanger_stm_tx_t*)pthread_getspecific(threadLocalKey)))
        tanger_stm_thread_init();
    return tx;
}


/**
 * Saves or restores the stack, depending on whether the current txn was
 * started or restarted. The STM will save/restore everything in the range
 * [low_addr, high_addr). The STM's implementation of this function must be
 * marked as no-inline, so it will get a new stack frame that does not
 * overlap the [low_addr, high_addr) region.
 * To avoid corrupting stack space of rollback functions, the STM should skip
 * undoing changes to addresses that are between the current stack pointer
 * during execution of the undo function and the [low_addr, high_addr)
 * area (i.e., all newer stack frames, including the current one).
 */
void tanger_stm_save_restore_stack(void* low_addr, void* high_addr)
{
    /* There are no aborts with the dummy STM, so we don't need to
     * save/restore the stack */
}

/**
 * Must be called by the thread after all transactions have finished.
 * FIXME this is never called. DummySTM should garbage-collect txns.
 */
void ITM_REGPARM tanger_stm_thread_shutdown()
{
	stm_tx_t* tx = (stm_tx_t*)tanger_stm_get_tx();
	free(tx);
}

static void _ITM_finalizeProcessCCallConv() { _ITM_finalizeProcess(); }

/**
 * Called after Tanger's STM support system has be initialized.
 */
int ITM_REGPARM _ITM_initializeProcess()
{
    pthread_mutexattr_t attr;
    int result = pthread_key_create(&threadLocalKey, NULL);
    if (result != 0) {
        perror("creating thread-local key");
        exit(1);
    }
    result = atexit(_ITM_finalizeProcessCCallConv);
    if (result != 0) {
        perror("registering atexit function");
        exit(1);
    }
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&globalLock, &attr);
    return 0;
}

/**
 * Called before Tanger's STM support system is shut down.
 */
void ITM_REGPARM _ITM_finalizeProcess()
{
	if (threadLocalInitialized) pthread_key_delete(threadLocalKey);
}

/**
 * Switches to another STM mode (e.g., serial-irrevocable).
 * We already have the global lock, and we don't support any other mode.
 */
void ITM_REGPARM _ITM_changeTransactionMode(_ITM_transactionState mode) {}


/**
 * Replacement function for malloc calls in transactions.
 */
void *_ITM_malloc(size_t size)
{
	return malloc(size);
}

/**
 * Replacement function for free calls in transactions.
 */
void _ITM_free(void *ptr)
{
	free(ptr);
}

/**
 * Replacement function for calloc calls in transactions.
 */
void *_ITM_calloc(size_t nmemb, size_t size)
{
	return calloc(nmemb, size);
}

/**
 * Replacement function for realloc calls in transactions.
 */
void *tanger_stm_realloc(void *ptr, size_t size)
{
	return realloc(ptr, size);
}
