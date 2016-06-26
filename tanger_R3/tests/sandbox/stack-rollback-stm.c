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

#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <ucontext.h>
#include <pthread.h>

typedef void tanger_stm_tx_t;

void tanger_stm_init() {}
void tanger_stm_shutdown() {}
void tanger_stm_thread_shutdown() {}

tanger_stm_tx_t* tanger_stm_get_tx() { return 0; }

sigjmp_buf jmpbuf;
void* tanger_stm_get_jmpbuf(tanger_stm_tx_t* tx) { return &jmpbuf; }

static int count = 0; 

void* tanger_stm_get_stack_area(tanger_stm_tx_t* tx, void* low, void* high)
{
	printf("low=%p high=%p\n", low, high);
	if (low > high) {
		printf("low/high assumption violated\n");
		exit(1);
	}
	count++;
	printf("size=%d count=%d\n", high-low, count);
	return malloc(high-low);
}

ucontext_t context;

// get some information about the stack
void tanger_stm_thread_init()
{
	if (getcontext(&context)) perror("getcontext");
	printf("ss_sp=%p\n", context.uc_stack.ss_sp);
	printf("flags=%d\n", context.uc_stack.ss_flags);
	printf("size=%d\n", context.uc_stack.ss_size);
	pthread_attr_t attr;
	void *saddr;
	size_t ssize;
	if (pthread_attr_init(&attr)) perror("attrinit");
	if (pthread_attr_getstack(&attr, &saddr, &ssize)) perror("getstack");
	printf("sadd=%p\n", saddr);
	printf("ssize=%d\n", ssize);
}


void tanger_stm_begin(tanger_stm_tx_t* tx)
{
	printf("begin\n");
}

void tanger_stm_commit(tanger_stm_tx_t* tx)
{
	if (count==2) {
		printf("abort\n");
		siglongjmp(jmpbuf, 1);
	}
	printf("commit\n");
}


