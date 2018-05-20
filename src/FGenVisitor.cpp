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

#include <FGenVisitor.hpp>
#include <util/Decl.hpp>

static bool isUserProvided(const clang::FunctionDecl *FunctionDecl)
{
    auto MethodDecl = clang::dyn_cast<clang::CXXMethodDecl>(FunctionDecl);

    return !MethodDecl || MethodDecl->isUserProvided();
}

FGenVisitor::FGenVisitor()
    : Targets_(), VisitedDecls_(), QualifiedNameBuffer_(), Output_()
{
    QualifiedNameBuffer_.reserve(1024);
    Output_.reserve(2048);
}

std::unordered_set<std::string> &FGenVisitor::targets()
{
    return Targets_;
}

const std::unordered_set<std::string> &FGenVisitor::targets() const
{
    return Targets_;
}

bool FGenVisitor::VisitFunctionDecl(clang::FunctionDecl *FunctionDecl)
{
    /*
     * 'VisitFunctionDecl' is a predefined interface by the clang library
     * which defines the entry points to the abstract syntax tree (AST).
     * We use it as a thin wrapper around the functionality to ensure
     * the correct return value here.
     */
    VisitFunctionDeclImpl(FunctionDecl);

    return true;
}

void FGenVisitor::dump(llvm::raw_ostream &OStream) const
{
    OStream << Output_ << "\n";
}

void FGenVisitor::VisitFunctionDeclImpl(const clang::FunctionDecl *FunctionDecl)
{
    auto &SM = FunctionDecl->getASTContext().getSourceManager();

    if (!SM.isInMainFile(FunctionDecl->getLocation()))
        return;

    if (!isUserProvided(FunctionDecl))
        return;

    if (!isTarget(FunctionDecl))
        return;

    /*
     * Not sure if this is really necessary:
     * The idea is to avoid to print the same function skeleton
     * multiple times.
     */
    auto USR = util::decl::generateUSR(FunctionDecl);

    if (VisitedDecls_.count(USR))
        return;

    VisitedDecls_.insert(std::move(USR));

    FunctionGenerator FunctionGenerator(Output_);
    FunctionGenerator.write(FunctionDecl);
}

bool FGenVisitor::isTarget(const clang::FunctionDecl *Decl)
{
    /*
     * If no 'RecordQualifier' is specified, all function declarations
     * are valid targets.
     */
    if (Targets_.empty())
        return true;

    auto MethodDecl = clang::dyn_cast<clang::CXXMethodDecl>(Decl);
    if (!MethodDecl)
        return false;

    QualifiedNameBuffer_.clear();

    llvm::raw_string_ostream OStream(QualifiedNameBuffer_);
    OStream.SetUnbuffered();

    MethodDecl->getParent()->printQualifiedName(OStream);

    return !!Targets_.count(QualifiedNameBuffer_);
}
