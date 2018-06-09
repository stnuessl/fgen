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

#ifndef FGEN_UTIL_DECL_HPP_
#define FGEN_UTIL_DECL_HPP_

#include <clang/AST/Decl.h>
#include <llvm/ADT/SmallVector.h>

namespace util {
namespace decl {

bool isSingleBit(const clang::FieldDecl *Decl);

void printFullQualifiedName(const clang::NamedDecl *NamedDecl,
                            llvm::raw_ostream &OStream,
                            bool WriteNamespaces = true);

llvm::StringRef getNameOrDefault(const clang::Decl *Decl,
                                 llvm::StringRef Default);

std::string generateUSR(const clang::Decl *Decl);

void getDeclContexts(const clang::NamedDecl *Decl,
                     llvm::SmallVectorImpl<const clang::DeclContext *> &Vec);
}
}

#endif /* FGEN_UTIL_DECL_HPP_ */
