/*
 * Copyright (C) 2018  Steffen Nüssle
 * fgen - function generator
 *
 * This file is part of fgen.
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

#ifndef FGEN_COMPILATIONDATABASE_HPP_
#define FGEN_COMPILATIONDATABASE_HPP_

#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/StringRef.h>

#include <clang/Tooling/CompilationDatabase.h>

#include <memory.h>

namespace util {
namespace compilation_database {

std::unique_ptr<clang::tooling::CompilationDatabase> load(llvm::StringRef Path,
                                                          std::string &ErrMsg);

std::unique_ptr<clang::tooling::CompilationDatabase>
detect(llvm::StringRef Directory, std::string &ErrMsg);

std::unique_ptr<clang::tooling::CompilationDatabase>
make(llvm::StringRef WorkingDirectory,
     llvm::ArrayRef<std::string> Files,
     std::string &ErrMsg);

bool contains(const clang::tooling::CompilationDatabase &Database,
              llvm::ArrayRef<std::string> Files);
}
}

#endif /* FGEN_COMPILATIONDATABASE_HPP_ */
