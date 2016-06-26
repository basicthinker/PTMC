// Copyright (C) 2009-2010  Martin Nowack
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation, version 2
// of the License.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
//
// A wrapper to ease the use of the transactional memory toolchain:
// Using Tanger and LLVM-GCC-TM
//
// For further information see README file.

include "llvm/CompilerDriver/Common.td"

def Options : OptionList<[
  (parameter_option "tangerpass", (help "set transformation pass"), (init "tanger")),
  (parameter_option "stmsupport", (help "path to stmsupport library")),
  (parameter_option "stmlib", (help "path to libtanger")),
  (switch_option "g", (help "enable debug information (disables optimizations)")),
  (prefix_list_option "l", (help "Add library when linking")),
  (prefix_list_option "L", (help "Search path for library")),
  (switch_option "n", (help "link with native compiled stm instead of llvm compiled one")),
  (prefix_list_option "f", (help "Additional compiler options")),
  (switch_option "static", (help "use static linking")),
  (switch_option "m32", (help "force creation of 32bit code")),
  (switch_option "disable-opt", (help "disable all optimizations")),
  (switch_option "force-opt", (help "force an additional optimization"))
]>;


// define used tools during the compilation process

// - links files together
def llvm_link_together : Tool<[
  (in_language "llvm-bitcode"),
  (out_language "llvm-bitcode"),
  (output_suffix "bc"),
  (command "llvm-ld -link-as-library"),
  (actions (case (switch_on "disable-opt"), (append_cmd "-disable-opt")
                 )),
  (join)
]>;

// - optimize files
def llvm_link_optimize : Tool<[
  (in_language "llvm-bitcode"),
  (out_language "llvm-bitcode"),
  (output_suffix "bc"),
  (command "llvm-ld -link-as-library"),
  (actions (case (switch_on "disable-opt"), (append_cmd "-disable-opt")
                 ))
]>;

//  (actions (case (not_empty "txnlibheaders"), (forward_as ""))),
// - linker executes opt command executing tanger pass
def llvm_tm_linker : Tool<[
  (in_language "llvm-bitcode"),
  (out_language "llvm-bitcode"),
  (output_suffix "bc"),
  (command "opt will be replaced with builtin pass execution")
]>;

// - add stm support to the binary file
def llvm_tm_stm_support : Tool<[
  (in_language "llvm-bitcode"),
  (out_language "llvm-bitcode"),
  (output_suffix "bc"),
  (command "llvm-ld -link-as-library $CALL(STMSUPPORTDIR)/stmsupport.bc"),
  (actions (case (empty "stmlib"), (error "-stmlib is not set"),
                 (empty "stmsupport"), (error "-stmsupport is not set"),
		 (switch_on "disable-opt"), (append_cmd "-disable-opt"),
                 (not (switch_on "n")),(append_cmd "$CALL(STMLIBDIR)/libtanger-stm.bc -internalize-public-api-file $CALL(STMLIBDIR)/libtanger-stm.public-symbols")
                 ))
]>;

// - tm compiler
def llvm_tm_compiler : Tool<[
  (in_language "llvm-bitcode"),
  (out_language "assembler"),
  (output_suffix "s"),
  (command "llc")
]>;

// - tm assembler
def llvm_tm_assembler : Tool<[
  (in_language "assembler"),
  (out_language "executable"),
  (output_suffix "out"),
  (command "g++ $CALL(STMLIBDIR)/libtanger-stm.a -pthread -x assembler"),
  (actions (case (empty "stmlib"), (error "-stmlib is not set"),
                 (not_empty "stmlib"), (append_cmd "-x none"),
                 (switch_on "g"), (forward "g"),
                 (not_empty "l"), (forward "l"),
                 (not_empty "f"), (forward "f"),
                 (not_empty "L"), (forward "L"),
                 (switch_on "static"), (forward "static"), 
		 (switch_on "static"), (append_cmd "-u pthread_create"),
                 (switch_on "m32"), (forward "m32"),
                 (switch_on "n"), (append_cmd "$CALL(STMLIBDIR)/libtstm.a"),
                 (not (switch_on "n")), (append_cmd "$CALL(STMLIBDIR)/libtanger-stm.a")
                 )),
  (sink)
]>;

// define language suffixes
def LanguageMap : LanguageMap<[
  (lang_to_suffixes "llvm-bitcode", ["bc", "o"]),
  (lang_to_suffixes "llvm-ir", "ll")
]>;


// define the graph which defines the correct execution order of the involved
// tools
def CompilationGraph : CompilationGraph<[
  (edge "root", "llvm_link_together"),
  (optional_edge "llvm_link_together", "llvm_link_optimize", (case (switch_on "force-opt"),(inc_weight))),
  (edge "llvm_link_together", "llvm_tm_linker"),
  (edge "llvm_link_optimize","llvm_tm_linker"),  
  (edge "llvm_tm_linker", "llvm_tm_stm_support"),
  (edge "llvm_tm_stm_support", "llvm_tm_compiler"),
  (edge "llvm_tm_compiler", "llvm_tm_assembler")
]>;
