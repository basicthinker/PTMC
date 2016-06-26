/* Copyright (C) 2008-2009  Torvald Riegel
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
#include <stdlib.h>
#include <stdio.h>
#include "tanger-stm-internal.h"

uint32_t tanger_stm_indirect_nb_targets_max_multi;
uint32_t tanger_stm_indirect_nb_targets_multi;
/* Number of versions per target. */
uint32_t tanger_stm_indirect_nb_versions;
/* Array with function pointer pairs: nontxnal, txn-v1, txn-v2, ..., nontxnal, ... */
void** tanger_stm_indirect_target_pairs_multi;

/**
 * Returns the transactional version of the function passed as argument.
 * If no transactional version has been registered, it triggers serial
 * irrevocable mode.
 */
void* tanger_stm_indirect_resolve_multiple(void *nontxnal_function, uint32_t version)
{
	uint32_t i;
	uint32_t max_nr = tanger_stm_indirect_nb_targets_multi * tanger_stm_indirect_nb_versions;
    for (i = 0; i < max_nr; i += tanger_stm_indirect_nb_versions) {
		if (tanger_stm_indirect_target_pairs_multi[i] == nontxnal_function)
			return tanger_stm_indirect_target_pairs_multi[i + version + 1];
	}
    /* no txnal version found, switch to serial-irrevocable mode and
     * call the original nontxnal version of the function. */
    _ITM_changeTransactionMode(modeSerialIrrevocable);
    return nontxnal_function;
}

/**
 * Called before transactional versions are registered for nontransactional
 * functions.
 * The parameter returns the exact number of functions that will be registered.
 */
void tanger_stm_indirect_init_multiple(uint32_t number_of_call_targets, uint32_t versions)
{
	tanger_stm_indirect_nb_targets_max_multi = number_of_call_targets;
	tanger_stm_indirect_nb_targets_multi = 0;

	tanger_stm_indirect_target_pairs_multi = calloc(number_of_call_targets * (versions + 1), sizeof(void*));
	tanger_stm_indirect_nb_versions = versions + 1;
}

/**
 * Registers a transactional versions for a nontransactional function.
 */
void tanger_stm_indirect_register_multiple(void* nontxnal, void* txnal, uint32_t version)
{
	uint32_t i;
	/* check if already registered */
	for(i = 0; i<tanger_stm_indirect_nb_targets_multi; ++i)
	{
		if (tanger_stm_indirect_target_pairs_multi[i*tanger_stm_indirect_nb_versions] == nontxnal)
		{
			tanger_stm_indirect_target_pairs_multi[i*tanger_stm_indirect_nb_versions + version + 1] = txnal;
			return;
		}
	}

	/* not found */
	tanger_stm_indirect_target_pairs_multi[tanger_stm_indirect_nb_targets_multi
	                                       *tanger_stm_indirect_nb_versions] = nontxnal;
	tanger_stm_indirect_target_pairs_multi[tanger_stm_indirect_nb_targets_multi
	                                       *tanger_stm_indirect_nb_versions + version + 1] = txnal;
	tanger_stm_indirect_nb_targets_multi += 1;
}
