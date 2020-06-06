/*
 * Copyright (C) 2018  Steffen Nüssle
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef FGEN_COMMANDLINE_HPP_
#define FGEN_COMMANDLINE_HPP_

#include <llvm/Support/raw_ostream.h>

namespace util {
namespace cl {

llvm::raw_ostream &error(llvm::raw_ostream &OS = llvm::errs());

llvm::raw_ostream &warning(llvm::raw_ostream &OS = llvm::errs());

llvm::raw_ostream &info(llvm::raw_ostream &OS = llvm::errs());
}
}

#endif /* FGEN_COMMANDLINE_HPP_ */
