/* Copyright (C) 2009-2010  Torvald Riegel
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
 * Supporting functions.
 */
#ifndef SUPPORT_H_
#define SUPPORT_H_

#include <llvm/Function.h>

namespace tanger {

llvm::raw_ostream& printWarning(const llvm::Function& f,
        const llvm::BasicBlock& bb);
llvm::raw_ostream& printWarning(const llvm::Function& f);
llvm::raw_ostream& printWarning();
llvm::raw_ostream& printError(const llvm::Function& f,
        const llvm::BasicBlock& bb);
llvm::raw_ostream& printError(const llvm::Function& f);
llvm::raw_ostream& printError();

/// Signal a fatal error.
void fatalError();
/// Check whether there was a fatal error.
bool hadFatalError();

} // namespace

#endif /* SUPPORT_H_ */
