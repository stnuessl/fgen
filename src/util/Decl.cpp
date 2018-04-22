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

#include <clang/Index/USRGeneration.h>

#include "Decl.hpp"

namespace util {
namespace decl {

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
