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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define compiler_barrier() __asm__ __volatile__("" : : : "memory")

static double tanger_wrapperpure_log(double) __attribute__ ((weakref("log")));
static float tanger_wrapperpure_tanger_stm_intrinsic_log_f32(float)
    __attribute__ ((weakref("tanger.llvm.log.f32")));
static double tanger_wrapperpure_tanger_stm_intrinsic_log_f64(double)
    __attribute__ ((weakref("tanger.llvm.log.f64")));

void test()
{
    double a01 = log(10);
//    double a02 = log(2);
//    double a03 = log(3);
//    double a04 = log(4);
//    double a05 = log(5);
//    double a06 = log(6);
//    double a07 = log(7);
//    double a08 = log(8);
//    double a09 = log(9);
    int sum = 0;

    compiler_barrier();
	__transaction {
        a01 *= log(1.2);
//        a02 *= log(1.2);
//        a03 *= log(1.8);
//        a04 *= log(1.6);
//        a05 *= log(1.4);
//        a06 *= log(1.3);
//        a07 *= log(1.1);
//        a08 *= log(1.2);
//        a09 *= log(1.5);
    }
    compiler_barrier();
    sum += a01;
//    sum += a02;
//    sum += a03;
//    sum += a04;
//    sum += a05;
//    sum += a06;
//    sum += a07;
//    sum += a08;
//    sum += a09;
	printf("sum=%d\n", sum);
}

int main(int argc, char** argv)
{
	test();
	return 0;
}
