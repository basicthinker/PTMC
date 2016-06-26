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
#include <stdint.h>
#include <stdlib.h>
#include <tanger-stm.h>

void txnal()
{
	printf("txnal\n");
}

void test()
{
	__asm__ volatile ("jmp 0f\n\tcli\n\tsti\n\tcli\n\tsti\n\tcli\n\t"
		"mov $0x1234, %eax\n0:");
	int x = 31;
	printf("x%d\n", x);
}

uint32_t pre[4] = {0xfbfa0aeb, 0xb8fafbfa};

void* find(uint8_t* func, uint32_t* prefix, uint32_t prefixLength)
{
//	uint32_t w1 = 0xfbfa0aeb;
//	uint32_t w2 = 0xb8fafbfa;
	int i,j;
	for (i = 0; i < 16; i++, func++) {
		if (*(uint32_t*)func == prefix[0]) {
			uint32_t* y = (uint32_t*)func;
			for (j = 1; j < prefixLength; j++) if (y[j] != prefix[j]) break;
			if (j == prefixLength) return (void*)*(uintptr_t*)(y+j);
		}
	}
	abort();   
}

int x = 0;

void call_target()
{
	x = 1;
}

void call_target_nontxnal()
{
	x = 1;
	printf("nontxnal\n");
}

void (*target)();

void call_test()
{
	tanger_begin();
	(*target)();
	tanger_commit();
	printf("x=%d\n", x);
}

int main(int argc, char** argv)
{
	test();
	printf("found:%p\n", find((uint8_t*)&test, pre, 2));

	tanger_init();
	tanger_thread_init();
	target = &call_target;
	call_test();
	target = &call_target_nontxnal;
	call_test();
	tanger_thread_shutdown();
	tanger_shutdown();
}
