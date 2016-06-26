/* Copyright (C) 2007-2010  Torvald Riegel
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
 * A dummy STM that uses a single lock and prints debug messages if necessary.
 * 
 * If you have any code that cannot be compiled by LLVM, you can use plain gcc
 * to compile this. If you don't have any such code, you should build the
 * (empty) binary library nonetheless to make the build process easier.
 */

#include "dummystm.h"
#include "tanger-stm-stats.h"

/* No reporting support. */
void* tanger_stm_report_start(const char* name) { return 0; }
void* tanger_stm_report_start_object(void* handle, const char* name) { return 0; }
void* tanger_stm_report_start_array(void* handle, const char* name) { return 0; }
void tanger_stm_report_append_long(void* handle, const char* name, int64_t i) {}
void tanger_stm_report_append_int(void* handle, const char* name, int i) {}
void tanger_stm_report_append_bool(void* handle, const char* name, int i) {}
void tanger_stm_report_append_double(void* handle, const char* name, double d) {}
void tanger_stm_report_append_string(void* handle, const char* name, const char* str) {}
void tanger_stm_report_finish_object(void* handle) {}
void tanger_stm_report_finish_array(void* handle) {}
void tanger_stm_report_finish(void* handle, const char* name) {}

