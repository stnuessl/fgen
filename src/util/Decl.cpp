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

void printFullQualifiedName(const clang::NamedDecl *Decl,
                            llvm::raw_ostream &OStream,
                            bool SkipNamespaces)
{
    /*
     * Write the qualified name of a named declaration including
     * template arguments.
     */
    llvm::SmallVector<const clang::DeclContext *, 8> DeclContextVec;

    util::decl::getDeclContexts(Decl, DeclContextVec);

    /* Walk from to top declaration down to 'Decl' */

    for (const auto DeclContext : DeclContextVec) {
        if (SkipNamespaces && clang::isa<clang::NamespaceDecl>(DeclContext))
            continue;

        /*
         * Just writing the Constructor / Destructor Decl to the stream
         * does not work in case we are dealing with a
         * 'ClassTemplateDecl': it would result in something like (1)
         *      class<T>::class<T>()
         *                ^(1)
         * which does not compile.
         * We handle these cases here for constructors and destructors
         * separately.
         */

        auto RecordDecl = clang::dyn_cast<clang::RecordDecl>(DeclContext);
        auto CtorDecl = clang::dyn_cast<clang::CXXConstructorDecl>(DeclContext);
        auto DtorDecl = clang::dyn_cast<clang::CXXDestructorDecl>(DeclContext);
        auto NamedDecl = clang::dyn_cast<clang::NamedDecl>(DeclContext);

        if (RecordDecl) {
            /* This takes care of the type name and the template arguments */
            auto &PrintingPolicy = Decl->getASTContext().getPrintingPolicy();

            auto Type = clang::QualType(RecordDecl->getTypeForDecl(), 0);
            Type.print(OStream, PrintingPolicy);
        } else if (CtorDecl) {
            OStream << *CtorDecl->getParent();
        } else if (DtorDecl) {
            OStream << '~' << *DtorDecl->getParent();
        } else {
            OStream << *NamedDecl;
        }

        if (DeclContext != DeclContextVec.back())
            OStream << "::";
    }
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

void getDeclContexts(const clang::NamedDecl *Decl,
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
}
}
