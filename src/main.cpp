/*
 * Copyright (C) 2017  Steffen Nüssle
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

// #include <clang/Frontend/TextDiagnosticPrinter.h>
#include <clang/Frontend/FrontendActions.h>
// #include <clang/Rewrite/Core/Rewriter.h>
// #include <clang/Tooling/Refactoring.h>
// #include <clang/Tooling/CompilationDatabase.h>
#include <clang/Tooling/JSONCompilationDatabase.h>
#include <clang/Tooling/Tooling.h>

#include "clang/Tooling/CommonOptionsParser.h"
#include <llvm/Support/CommandLine.h>

#include "FunctionGenerator.hpp"

/* clang-format off */

static llvm::cl::OptionCategory FGenOptions("Function Generating Options");

static llvm::cl::list<std::string> TargetVec(
    "target",
    llvm::cl::desc(
        "Generate function bodys for [target...]. [target...] must be\n"
        "fully qualified identifiers, e.g. \"my_namespace::my_class\"."
    ),
    llvm::cl::value_desc("target"),
    llvm::cl::CommaSeparated,
    llvm::cl::cat(FGenOptions)
);

/* clang-format on */



class MethodGeneratorAction : public clang::ASTFrontendAction {
public:
    virtual std::unique_ptr<clang::ASTConsumer>
    CreateASTConsumer(clang::CompilerInstance &CI,
                      llvm::StringRef InFile) override;
};

std::unique_ptr<clang::ASTConsumer>
MethodGeneratorAction::CreateASTConsumer(clang::CompilerInstance &CI,
                                         llvm::StringRef InFile)
{
    class ASTConsumer : public clang::ASTConsumer {
    public:
        virtual void HandleTranslationUnit(clang::ASTContext &Context) override
        {
            auto MGen = FunctionGenerator();
            
            auto Begin = std::make_move_iterator(TargetVec.begin());
            auto End = std::make_move_iterator(TargetVec.end());
            MGen.targets().insert(Begin, End);

            MGen.TraverseDecl(Context.getTranslationUnitDecl());
            MGen.dump();
        }
    };

    return llvm::make_unique<ASTConsumer>();
}

int main(int argc, const char **argv)
{
    using namespace clang::tooling;

    CommonOptionsParser Parser(argc, argv, FGenOptions);

    ClangTool Tool(Parser.getCompilations(), Parser.getSourcePathList());

    return Tool.run(newFrontendActionFactory<MethodGeneratorAction>().get());
}
