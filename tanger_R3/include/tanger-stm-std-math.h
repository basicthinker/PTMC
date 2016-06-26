/* Copyright (C) 2008  Torvald Riegel
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
 * Wrapper declarations for math.h
 */
#ifndef TANGERSTMSTDMATH_H_
#define TANGERSTMSTDMATH_H_

#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

/* FIXME log et al change errno, read FPU flags, rounding mode, ... */
double log(double) __attribute__ ((transaction_pure));
double sqrt(double) __attribute__ ((transaction_pure));
double acos(double) __attribute__ ((transaction_pure));
double cos(double) __attribute__ ((transaction_pure));
float fcos(float) __attribute__ ((transaction_pure));
double sin(double) __attribute__ ((transaction_pure));
float fsin(float) __attribute__ ((transaction_pure));
double fabs(double) __attribute__ ((transaction_pure));

/* intrinsics */
/* FIXME log et al change errno, read FPU flags, rounding mode, ... */
static double tanger_wrapperpure_tanger_stm_intrinsic_log_f64(double)
    __attribute__ ((weakref("tanger.llvm.log.f64")));
static float tanger_wrapperpure_tanger_stm_intrinsic_log_f32(float)
    __attribute__ ((weakref("tanger.llvm.log.f32")));
static double tanger_wrapperpure_tanger_stm_intrinsic_sqrt_f64(double)
    __attribute__ ((weakref("tanger.llvm.sqrt.f64")));
static float tanger_wrapperpure_tanger_stm_intrinsic_sqrt_f32(float)
    __attribute__ ((weakref("tanger.llvm.sqrt.f32")));

#ifdef __cplusplus
}
#endif

#endif /*TANGERSTMSTDMATH_H_*/
