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

#include <string>

#include <clang/Index/USRGeneration.h>
#include "clang/Tooling/CommonOptionsParser.h"

#include <llvm/Support/CommandLine.h>

#include "FunctionGenerator.hpp"

namespace {

std::string getUnifiedSymbolResolution(const clang::Decl *Decl)
{
    llvm::SmallVector<char, 128> USRBuffer;

    clang::index::generateUSRForDecl(Decl, USRBuffer);

    return std::string(USRBuffer.begin(), USRBuffer.end());
}

void getQualifiedName(const clang::NamedDecl *Decl, 
                      std::string &Name, 
                      bool IgnoreNamespaces = false)
{
    auto OldSize = Name.size();

    auto StrRef = Decl->getName();
    
    for (size_t i = 0, Size = StrRef.size(); i < Size; ++i)
        Name += StrRef[Size - i - 1];
    
    Name += "::";

    auto DeclContext = Decl->getDeclContext();
    while (DeclContext) {
        auto NamedDecl = clang::dyn_cast<clang::NamedDecl>(DeclContext);
        if (!NamedDecl)
            break;
            
        if (IgnoreNamespaces && clang::isa<clang::NamespaceDecl>(NamedDecl))
            break;
        
        auto StrRef = NamedDecl->getName();
        for (size_t i = 0, Size = StrRef.size(); i < Size; ++i)
            Name += StrRef[Size - i - 1];
        
        Name += "::";

        DeclContext = DeclContext->getParent();
    }
    
    Name.pop_back();
    Name.pop_back();
    
    std::reverse(std::next(Name.begin(), OldSize), Name.end());
}

void getDeclContexts(const clang::Decl *Decl, 
                     std::vector<const clang::DeclContext *> &Vec)
{
    auto OldSize = Vec.size();
    
    auto DeclContext = Decl->getDeclContext();
    while (DeclContext) {
        Vec.push_back(DeclContext);
        DeclContext = DeclContext->getParent();
    }
    
    std::reverse(std::next(Vec.begin(), OldSize), Vec.end());
}

bool isConstructor(const clang::FunctionDecl *Decl)
{
    return clang::isa<clang::CXXConstructorDecl>(Decl);
}

bool isDestructor(const clang::FunctionDecl *Decl)
{
    return clang::isa<clang::CXXDestructorDecl>(Decl);
}

} // namespace

FunctionGenerator::FunctionGenerator()
    : Buffer_(),
      Targets_(),
      VisitedDecls_(),
      LangOptions_(),
      PrintingPolicy_(LangOptions_),
      Output_(),
      IgnoreNamespaces_(true)
{
    Buffer_.QualifiedName.reserve(1024);
    Output_.reserve(2048);

    PrintingPolicy_.adjustForCPlusPlus();
}

std::unordered_set<std::string> &FunctionGenerator::targets()
{
    return Targets_;
}

const std::unordered_set<std::string> &FunctionGenerator::targets() const
{
    return Targets_;
}


bool FunctionGenerator::VisitFunctionDecl(clang::FunctionDecl *FunctionDecl)
{
    /*
     *'VisitFunctionDecl' is a predefined interface by the clang library
     * which defines the entry points to the abstract syntax tree (AST).
     * We use it as a thin wrapper around the functionality to ensure
     * the correct return value here.
     */
    VisitFunctionDeclImpl(FunctionDecl);

    return true;
}

void FunctionGenerator::dump(llvm::raw_ostream &OStream) const
{
    OStream << Output_ << "\n";
}

void FunctionGenerator::VisitFunctionDeclImpl(
    const clang::FunctionDecl *FunctionDecl)
{
    auto &SM = FunctionDecl->getASTContext().getSourceManager();

    if (!SM.isInMainFile(FunctionDecl->getLocation()))
        return;

    if (!isTarget(FunctionDecl))
        return;

    /* 
     * Not sure if this is really necessary:
     * The idea is to avoid to print the same function skeleton
     * multiple times.
     */
    auto USR = getUnifiedSymbolResolution(FunctionDecl);

    if (VisitedDecls_.count(USR))
        return;

    VisitedDecls_.insert(std::move(USR));

    writeFunction(FunctionDecl);
}

void FunctionGenerator::writeFunction(const clang::FunctionDecl *FunctionDecl)
{
    writeTemplateParameters(FunctionDecl);
    writeReturnType(FunctionDecl);
    writeQualifiedName(FunctionDecl);
    writeParameters(FunctionDecl);
    writeBody(FunctionDecl);
}

void FunctionGenerator::writeTemplateParameters(
    const clang::FunctionDecl *FunctionDecl)
{
    Buffer_.DeclContextVec.clear();
    getDeclContexts(FunctionDecl, Buffer_.DeclContextVec);

    for (const auto DeclContext : Buffer_.DeclContextVec) {
        auto CXXRecordDecl = clang::dyn_cast<clang::CXXRecordDecl>(DeclContext);
        if (!CXXRecordDecl)
            continue;

        auto ClassTemplateDecl = CXXRecordDecl->getDescribedClassTemplate();
        if (!ClassTemplateDecl)
            continue;

        auto TemplateParams = ClassTemplateDecl->getTemplateParameters();
        writeTemplateParameters(TemplateParams);
    }

    auto FunctionTemplateDecl = FunctionDecl->getDescribedFunctionTemplate();
    if (!FunctionTemplateDecl)
        return;

    auto TemplateParams = FunctionTemplateDecl->getTemplateParameters();
    writeTemplateParameters(TemplateParams);
}

void FunctionGenerator::writeTemplateParameters(
    const clang::TemplateParameterList *List)
{
    Output_ += "template <";

    for (const auto Decl : *List) {
        auto TTPDecl = clang::dyn_cast<clang::TemplateTypeParmDecl>(Decl);
        auto NonTTPDecl = clang::dyn_cast<clang::NonTypeTemplateParmDecl>(Decl);
        
        if (TTPDecl) {
            Output_ += "typename ";
            
            if (TTPDecl->isParameterPack())
                Output_ += "... ";
            
        } else if (NonTTPDecl) {
            Output_ += NonTTPDecl->getType().getAsString(PrintingPolicy_);
            Output_ += " ";
        }

        Output_ += Decl->getName();
        Output_ += ", ";
    }
    
    /* Remove the last ", " which gets added for each loop iteration */
    if (Output_.back() == ' ') {
        Output_.pop_back();
        Output_.pop_back();
    }

    Output_ += ">\n";
}

void FunctionGenerator::writeReturnType(const clang::FunctionDecl *FunctionDecl)
{
    if (isConstructor(FunctionDecl) || isDestructor(FunctionDecl))
        return;

    Output_ += FunctionDecl->getReturnType().getAsString(PrintingPolicy_);

    auto LastChar = Output_.back();
    if (LastChar != ' ' && LastChar != '&' && LastChar != '*')
        Output_ += " ";
}

void FunctionGenerator::writeQualifiedName(
    const clang::FunctionDecl *FunctionDecl)
{
    Buffer_.QualifiedName.clear();
    getQualifiedName(FunctionDecl, Buffer_.QualifiedName, IgnoreNamespaces_);
    Output_ += Buffer_.QualifiedName;
}

void FunctionGenerator::writeParameters(const clang::FunctionDecl *FunctionDecl)
{
    unsigned int ArgCount = 0;

    Output_ += "(";

    for (const auto &Param : FunctionDecl->parameters()) {
        auto TypeName = Param->getType().getAsString();

        auto Index = TypeName.rfind("::");
        if (Index != std::string::npos) {
            /*
             * Given a type string like:
             *      std::vector::size_type
             * Reduce it to just:
             *      size_type
             *
             * The fully qualified function name makes sure that the compiler
             * will be able to resolve the correct type.
             */
            auto Begin = TypeName.begin();
            auto End = std::next(Begin, Index + std::strlen("::"));

            TypeName.erase(Begin, End);
        }

        Output_ += TypeName;

        /*
         * Output for reference and pointer types:
         *      "type &"
         *      "type *"
         * Output for other types:
         *      "type "
         */
        auto LastChar = Output_.back();
        if (LastChar != ' ' && LastChar != '&' && LastChar != '*')
            Output_ += " ";

        if (Param->getName().empty()) {
            Output_ += "arg";
            Output_ += std::to_string(++ArgCount);
        } else {
            Output_ += Param->getName();
        }

        Output_ += ", ";
    }

    /* Did we go into the above for loop? If yes, remove the trailing ", " */
    if (Output_.back() != '(') {
        Output_.pop_back();
        Output_.pop_back();
    }

    Output_ += ")\n";
}

void FunctionGenerator::writeBody(const clang::FunctionDecl *FunctionDecl)
{
    Output_ += "{\n\n}\n\n";
}

bool FunctionGenerator::isTarget(const clang::FunctionDecl *Decl)
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
    
    Buffer_.QualifiedName.clear();
    getQualifiedName(MethodDecl->getParent(), Buffer_.QualifiedName);

    return !!Targets_.count(Buffer_.QualifiedName);
}
