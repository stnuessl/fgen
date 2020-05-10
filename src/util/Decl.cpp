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
#include <clang/Index/USRGeneration.h>

#include "Decl.hpp"

namespace util {
namespace decl {

bool isSingleBit(const clang::FieldDecl *FieldDecl)
{
    if (!FieldDecl->isBitField())
        return false;

    return FieldDecl->getBitWidthValue(FieldDecl->getASTContext()) == 1;
}

void getQualifiedName(const clang::NamedDecl *NamedDecl, std::string &Buffer)
{
    llvm::raw_string_ostream OStream(Buffer);

    NamedDecl->printQualifiedName(OStream);
}

llvm::StringRef getNameOrDefault(const clang::Decl *Decl,
                                 llvm::StringRef Default)
{
    auto NamedDecl = clang::dyn_cast<clang::NamedDecl>(Decl);
    if (!NamedDecl)
        return Default;

    auto Name = NamedDecl->getName();
    return (Name.empty()) ? Default : Name;
}

std::string generateUSR(const clang::Decl *Decl)
{
    llvm::SmallString<128> USRBuffer;

    clang::index::generateUSRForDecl(Decl, USRBuffer);

    return std::string(USRBuffer.begin(), USRBuffer.end());
}

void getFullContext(const clang::NamedDecl *Decl,
                    llvm::SmallVectorImpl<const clang::DeclContext *> &Vec)
{
    auto OldSize = Vec.size();

    auto DeclContext = clang::dyn_cast<clang::DeclContext>(Decl);
    if (DeclContext)
        Vec.push_back(DeclContext);

    DeclContext = Decl->getDeclContext();
    while (DeclContext) {
        if (clang::isa<clang::NamedDecl>(DeclContext))
            Vec.push_back(DeclContext);

        DeclContext = DeclContext->getParent();
    }

    /* Just reverse the part that we added to the vector */
    std::reverse(std::next(std::begin(Vec), OldSize), std::end(Vec));
}

bool hasReturnType(const clang::FunctionDecl *FunctionDecl)
{
    switch (FunctionDecl->getKind()) {
    case clang::Decl::CXXConstructor:
    case clang::Decl::CXXDestructor:
    case clang::Decl::CXXConversion:
        return false;
    default:
        return true;
    }
}

bool hasTrailingReturnType(const clang::FunctionDecl *FunctionDecl)
{
    if (!hasReturnType(FunctionDecl))
        return false;

    auto Loc = FunctionDecl->getReturnTypeSourceRange().getBegin();
    auto ReturnType = FunctionDecl->getReturnType();
    auto DeclaredReturnType = FunctionDecl->getDeclaredReturnType();
    
    return Loc.isInvalid() || ReturnType != DeclaredReturnType;
}

}
}
