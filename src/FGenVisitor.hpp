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

#ifndef FGEN_FGENVISITOR_HPP_
#define FGEN_FGENVISITOR_HPP_

#include <clang/AST/RecursiveASTVisitor.h>
#include <unordered_set>

#include <FunctionGenerator.hpp>

class FGenVisitor : public clang::RecursiveASTVisitor<FGenVisitor> {
public:
    FGenVisitor();

    std::unordered_set<std::string> &targets();
    const std::unordered_set<std::string> &targets() const;

    bool VisitFunctionDecl(clang::FunctionDecl *FunctionDecl);

    void dump(llvm::raw_ostream &OStream = llvm::outs()) const;

private:
    void VisitFunctionDeclImpl(const clang::FunctionDecl *FunctionDecl);

    bool isTarget(const clang::FunctionDecl *Decl);

    std::unordered_set<std::string> Targets_;
    std::unordered_set<std::string> VisitedDecls_;

    std::string QualifiedNameBuffer_;
    std::string Output_;
};

#endif /* FGEN_FGENVISITOR_HPP_ */
