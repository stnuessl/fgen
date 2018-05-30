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

#ifndef FGEN_UTIL_TYPE_HPP_
#define FGEN_UTIL_TYPE_HPP_

#include <clang/AST/Type.h>

namespace util {
namespace type {

// clang::QualType getPointeeType(clang::QualType Type);

clang::QualType removeConst(clang::QualType Type);

clang::QualType getNonConstNonReferenceType(clang::QualType Type);

bool isReturnAssignmentOk(clang::QualType Type1, clang::QualType Type2);

bool isVariableAssignmentOk(clang::QualType Type1, clang::QualType Type2);

bool hasDefaultConstructor(const clang::QualType Type);

bool hasMoveAssignment(const clang::QualType Type);
}
}

#endif /* FGEN_UTIL_TYPE_HPP_ */
