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

#include <clang/AST/DeclCXX.h>

#include <util/Type.hpp>

namespace util {
namespace type {

clang::QualType removeConst(clang::QualType Type)
{
    if (!Type.isConstQualified())
        return Type;

    auto Qualifiers = Type.getQualifiers();

    Qualifiers.removeConst();

    return clang::QualType(Type.getTypePtr(), Qualifiers.getAsOpaqueValue());
}

clang::QualType getNonConstNonReferenceType(clang::QualType Type)
{
    return removeConst(Type.getNonReferenceType());
}


bool isReturnAssignmentOk(clang::QualType Type1, clang::QualType Type2)
{
    /*
     * Perform a check if type2 can be assigned to type1 without a
     * warning or error during compilation.
     */

    if (Type1->isPointerType() || Type1->isReferenceType()) {
        /*
         * You can assign both, const and non-const values to a const reference
         * or pointer, but you cannot assign a const reference or pointer type
         * to a * non-const type of the same kind.
         *
         * Assigning an inherited class to its base class is not considered.
         */
        bool ConstOk = Type1.isConstQualified() || !Type2.isConstQualified();
        Type1 = getNonConstNonReferenceType(Type1).getCanonicalType();
        Type2 = getNonConstNonReferenceType(Type2).getCanonicalType();

        return Type1 == Type2 && ConstOk;
    }

    return Type1.getCanonicalType() == Type2.getCanonicalType();
}

bool isVariableAssignmentOk(clang::QualType Type1, clang::QualType Type2)
{
    bool Type1Const = Type1.isConstQualified();
    
    if (Type1Const && !Type1->isPointerType() && !Type1->isReferenceType())
        return false;

    return isReturnAssignmentOk(Type1, Type2);
}

bool hasDefaultConstructor(const clang::QualType Type)
{
    auto RecordType = Type->getAs<clang::RecordType>();
    if (!RecordType)
        return false;

    auto Decl = clang::dyn_cast<clang::CXXRecordDecl>(RecordType->getDecl());

    return Decl && Decl->hasDefaultConstructor();
}

bool hasMoveAssignment(const clang::QualType Type)
{
    auto RecordType = Type->getAs<clang::RecordType>();
    if (!RecordType)
        return false;

    auto Decl = clang::dyn_cast<clang::CXXRecordDecl>(RecordType->getDecl());

    return Decl && Decl->hasMoveAssignment();
}
}
}
