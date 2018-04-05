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

#ifndef FGEN_FUNCTION_GENERATOR_HPP_
#define FGEN_FUNCTION_GENERATOR_HPP_

#include <unordered_set>

#include <clang/AST/RecursiveASTVisitor.h>

class FunctionGenerator : public clang::RecursiveASTVisitor<FunctionGenerator> {
public:
    FunctionGenerator();
    
    std::unordered_set<std::string> &targets();
    const std::unordered_set<std::string> &targets() const;

    bool VisitFunctionDecl(clang::FunctionDecl *FunctionDecl);

    void dump(llvm::raw_ostream &OStream = llvm::outs()) const;

private:
    void VisitFunctionDeclImpl(const clang::FunctionDecl *FunctionDecl);

    void writeFunction(const clang::FunctionDecl *FunctionDecl);
    void writeTemplateParameters(const clang::FunctionDecl *FunctionDecl);
    void writeTemplateParameters(const clang::TemplateParameterList *List);
    void writeReturnType(const clang::FunctionDecl *FunctionDecl);
    void writeQualifiedName(const clang::FunctionDecl *FunctionDecl);
    void writeParameters(const clang::FunctionDecl *FunctionDecl);
    void writeBody(const clang::FunctionDecl *FunctionDecl);

    bool isTarget(const clang::FunctionDecl *Decl);

    /*
     * A structure which holds elements which help to minimize
     * new allocations and reuse already allocated memory.
     */
    struct {
        std::vector<const clang::DeclContext *> DeclContextVec;
        std::string QualifiedName;
    } Buffer_;

    std::unordered_set<std::string> Targets_;
    std::unordered_set<std::string> VisitedDecls_;

    clang::LangOptions LangOptions_;
    clang::PrintingPolicy PrintingPolicy_;

    std::string Output_;
    
    bool IgnoreNamespaces_;
};

#endif /* FGEN_FUNCTION_GENERATOR_HPP_ */
