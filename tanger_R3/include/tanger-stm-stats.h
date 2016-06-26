/*
 * Author(s):
 *  Jons-Tobias Wamhoff <jons@inf.tu-dresden.de>
 *  Martin Nowack <martin.nowack@inf.tu-dresden.de>
 *
 * Description:
 *   Internal STM interface for statistics.
 *
 * Copyright (c) 2010.
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

#ifndef TANGERSTMSTATS_H_
#define TANGERSTMSTATS_H_

#ifdef __cplusplus
extern "C" {
#endif

/** Notify the STM about the begin of an experiment, e.g., to start time measurement. */
void tanger_stm_stats_begin_experiment();
/** Notify the STM about the end of an experiment, e.g., to stop time measurement. */
void tanger_stm_stats_end_experiment();
/** Enable statistics at runtime, e.g., event capturing. */
void tanger_stm_stats_enable();
/** Disable statistics at runtime. */
void tanger_stm_stats_disable();
/** Capture an external event */
void tanger_stm_stats_external_event();

/** Returns a handle. */
void* tanger_stm_report_start(const char* name);
/** Returns a handle. */
void* tanger_stm_report_start_object(void* handle, const char* name);
void* tanger_stm_report_start_array(void* handle, const char* name);
void tanger_stm_report_append_long(void* handle, const char* name, int64_t i);
void tanger_stm_report_append_int(void* handle, const char* name, int i);
void tanger_stm_report_append_bool(void* handle, const char* name, int b);
void tanger_stm_report_append_double(void* handle, const char* name, double d);
void tanger_stm_report_append_string(void* handle, const char* name, const char* str);
void tanger_stm_report_finish_object(void* handle);
void tanger_stm_report_finish_array(void* handle);
/** Use same name as for tanger_stm_report_start. */
void tanger_stm_report_finish(void* handle, const char* name);


#ifdef __cplusplus
}
#endif

#endif /* TANGERSTMSTATS_H_ */
