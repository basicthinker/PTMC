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
 * Supporting functions.
 */
#include <llvm/Support/raw_ostream.h>
#include "support.h"

using namespace tanger;
using namespace llvm;

namespace {
bool fatalErrorFlag = false;
}

namespace tanger {

raw_ostream& printWarning(const Function& f, const BasicBlock& bb)
{
    errs() << "WARNING in function '" << f.getName() << "' block '"
            << bb.getName() << "': ";
    return errs();
}
raw_ostream& printWarning(const Function& f)
{
    errs() << "WARNING in function '" << f.getName() << "': ";
    return errs();
}
raw_ostream& printWarning()
{
    errs() << "WARNING: ";
    return errs();
}

raw_ostream& printError(const Function& f, const BasicBlock& bb)
{
    errs() << "ERROR in function '" << f.getName() << "' block '"
            << bb.getName() << "': ";
    return errs();
}

raw_ostream& printError(const Function& f)
{
    errs() << "ERROR in function '" << f.getName() << "': ";
    return errs();
}

raw_ostream& printError()
{
    errs() << "ERROR: ";
    return errs();
}

void fatalError()
{
    fatalErrorFlag = true;
}
bool hadFatalError()
{
    return fatalErrorFlag;
}

}
