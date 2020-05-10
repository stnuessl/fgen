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

// clang::QualType getPointeeType(clang::QualType Type);
// {
//     if (Type->isPointerType() || Type->isReferenceType())
//         Type = Type->getPointeeType();
//
//     if ()
//     Type = Type.getNonReferenceType();
//
//     if
//
// }

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

bool returnAssignmentOk(clang::QualType LHS, clang::QualType RHS)
{
    /*
     * Perform a check if "RHS" can be assigned to "LHS" without a
     * warning or error during compilation.
     */

    if (LHS->isPointerType() || LHS->isReferenceType()) {
        /*
         * You can assign both, const and non-const values to a const reference
         * or pointer, but you cannot assign a const reference or pointer type
         * to a non-const type of the same kind.
         *
         * Assigning an inherited class to its base class is not considered.
         */
        bool ConstOk = LHS.isConstQualified() || !RHS.isConstQualified();

        LHS = getNonConstNonReferenceType(LHS).getCanonicalType();
        RHS= getNonConstNonReferenceType(LHS).getCanonicalType();

        return LHS == RHS && ConstOk;
    }

    return LHS.getCanonicalType() == RHS.getCanonicalType();
}

bool variableAssignmentOk(clang::QualType LHS, clang::QualType RHS)
{
    bool LHSIsConst = LHS.isConstQualified();

    /* Assignments to const pointers / references are possible. */
    if (LHSIsConst && !LHS->isPointerType() && !LHS->isReferenceType())
        return false;

    return returnAssignmentOk(LHS, RHS);
}

bool hasDefaultConstructor(const clang::QualType Type)
{
    auto RecordType = Type->getAs<clang::RecordType>();
    if (!RecordType)
        return false;

    auto Decl = clang::dyn_cast<clang::CXXRecordDecl>(RecordType->getDecl());
    if (!Decl)
        return false;

    auto Pred = [](const clang::CXXConstructorDecl *CtorDecl) {
        return CtorDecl->isDefaultConstructor() && !CtorDecl->isDeleted();
    };

    return llvm::any_of(Decl->ctors(), Pred);
}

bool hasMoveAssignment(const clang::QualType Type)
{
    auto RecordType = Type->getAs<clang::RecordType>();
    if (!RecordType)
        return false;

    auto Decl = clang::dyn_cast<clang::CXXRecordDecl>(RecordType->getDecl());
    if (!Decl)
        return false;

    auto Pred = [](const clang::CXXMethodDecl *MDecl) {
        return MDecl->isMoveAssignmentOperator() && !MDecl->isDeleted();
    };

    return llvm::any_of(Decl->methods(), Pred);
}

}
}
