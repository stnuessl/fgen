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

#ifndef FGEN_FUNCTIONGENERATOR_HPP_
#define FGEN_FUNCTIONGENERATOR_HPP_

#include <unordered_set>

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/DeclTemplate.h>
#include <llvm/Support/raw_ostream.h>

#include <FGenConfiguration.hpp>
#include <StringStream.hpp>

/*
 * Simple one pass class, which gets used for every valid
 * function declaration. Basically wraps a 'std::string' in
 * a 'llvm::raw_string_ostream' to make use of the handy
 * 'operator<<()' overloads.
 */

class FunctionGenerator {
public:
    FunctionGenerator();

    void setConfiguration(std::shared_ptr<FGenConfiguration> Configuration);

    void add(const clang::FunctionDecl *FunctionDecl);
    void dump(llvm::raw_ostream &OStream = llvm::outs()) const;
    void clear();

private:
    void writeNamespaceDefinitions(
        const llvm::SmallVector<const clang::DeclContext *, 8> &ContextVec);
    void writeTemplateParameters(const clang::FunctionDecl *FunctionDecl);
    void writeTemplateParameters(const clang::TemplateParameterList *List);
    void writeReturnType(const clang::FunctionDecl *FunctionDecl);
    void writeFullQualifiedName(
        const llvm::SmallVector<const clang::DeclContext *, 8> &ContextVec);
    void writeParameters(const clang::FunctionDecl *FunctionDecl);

    void writeQualifiers(const clang::FunctionDecl *FunctionDecl);
    void writeBody(const clang::FunctionDecl *FunctionDecl);
    void writeEnding();

    bool tryWriteConversionStatement(const clang::FunctionDecl *FunctionDecl);
    bool tryWriteReturnStatement(const clang::FunctionDecl *FunctionDecl);
    bool tryWriteGetAccessor(const clang::FunctionDecl *FunctionDecl);
    bool tryWriteCGetAccessor(const clang::FunctionDecl *FunctionDecl);
    bool tryWriteCXXGetAccessor(const clang::CXXMethodDecl *MethodDecl);

    bool tryWriteSetAccessor(const clang::FunctionDecl *FunctionDecl);
    bool tryWriteCSetAccessor(const clang::FunctionDecl *FunctionDecl);
    bool tryWriteCXXSetAccessor(const clang::CXXMethodDecl *MethodDecl);

    bool useMoveAssignment(clang::QualType Type);
    bool addInclude(std::string Include);

    std::vector<const clang::NamespaceDecl *> ActiveNamespaces_;
    std::unordered_set<std::string> Includes_;
    StringStream StrStream_;

    std::shared_ptr<FGenConfiguration> Configuration_;
};

#endif /* FGEN_FUNCTIONGENERATOR_HPP_ */
