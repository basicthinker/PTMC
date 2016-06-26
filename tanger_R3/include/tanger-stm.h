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
 * Application-level STM interface.
 *
 * This file contains the interface to the STM that is visible in the
 * application. Tanger transforms
 * We have chosen the "tanger_" prefix in the hope that it results in less
 * name collisions than "atomic*".
 */
#ifndef TANGERSTM_H_
#define TANGERSTM_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Markers for transaction begin/end.
 * These will be replaced by Tanger, so you must not implement these
 * functions.
 * Names are lowercase because uppercase is harder to type (although
 * it might be easier to spot).
 */
void __attribute__((nothrow)) tanger_begin(void);
void __attribute__((nothrow)) tanger_commit(void);

#ifdef __cplusplus
}
#endif

#endif /*TANGERSTM_H_*/
