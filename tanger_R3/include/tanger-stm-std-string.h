/* Copyright (C) 2008-2010  Torvald Riegel
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
 * Wrapper declarations for string.h
 */
#ifndef TANGERSTMSTDSTRING_H_
#define TANGERSTMSTDSTRING_H_

#include <string.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __APPLE__
void * memset(void *src, int c, size_t n) __attribute__ ((tm_wrapper("tanger_stm_std_memset")));
void * memcpy(void *dest, const void *src, size_t n) __attribute__ ((tm_wrapper("tanger_stm_std_memmove")));
void * memmove(void *dest, const void *src, size_t n) __attribute__ ((tm_wrapper("tanger_stm_std_memmove")));
#endif

#ifdef __cplusplus
}
#endif

#endif /*TANGERSTMSTDSTRING_H_*/
