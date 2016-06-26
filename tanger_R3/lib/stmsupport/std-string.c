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
#include <string.h>
#include "tanger-stm-internal.h"

extern void* tanger_stm_std_memset(void *src, int c, size_t n) __attribute__((weak));
extern void* tanger_stm_std_memset(void *src, int c, size_t n)
{
    _ITM_memsetW(src, c, n);
    return src;
}

extern void* tanger_stm_std_memcpy(void *dest, const void *src, size_t n) __attribute__((weak));
extern void* tanger_stm_std_memcpy(void *dest, const void *src, size_t n)
{
    _ITM_memcpyRtWt(dest, src, n);
    return dest;
}

extern void* tanger_stm_std_memmove(void *dest, const void *src, size_t n) __attribute__((weak));
extern void* tanger_stm_std_memmove(void *dest, const void *src, size_t n)
{
    _ITM_memmoveRtWt(dest, src, n);
    return dest;
}
