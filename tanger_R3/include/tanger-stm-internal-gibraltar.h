/* Copyright (C) 2008  Jons-Tobias Wamhoff
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
#ifndef TANGERSTMINTERNALGIBRALTAR_H_
#define TANGERSTMINTERNALGIBRALTAR_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize Gibraltar.
 * 
 * Called after the STM has been initialized.
 */
void tanger_stm_gibraltar_init();

/**
 * Notifies Gibraltar about a new thread.
 * 
 * Called after the STM has been notified about the new thread.
 * Needs to be called before tanger_stm_gibraltar_execute_instrumented() can
 * be used.
 */
void tanger_stm_gibraltar_thread_init();

/**
 * Switches from Probe to JIT mode. Subsequent instructions can
 * be dynamically instrumented.
 */
void tanger_stm_gibraltar_execute_instrumented();

/**
 * Switches from Probe to JIT mode. Subsequent instructions can
 * be dynamically instrumented. The current stack frame is known to
 * be transaction-local (i.e., it has been created in the transaction and
 * its addresses thus cannot have been shared later on with other
 * transactions). 
 */
void tanger_stm_gibraltar_execute_instrumented_txnlocalstackframe();

/**
 * Switches from JIT to Probe mode. Subsequent instructions are 
 * executed natively.
 */
void tanger_stm_gibraltar_execute_uninstrumented();

#ifdef __cplusplus
}
#endif

#endif /*TANGERSTMINTERNALGIBRALTAR_H_*/
