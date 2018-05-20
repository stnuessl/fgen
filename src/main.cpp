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
#include <clang/Tooling/Tooling.h>

#include "clang/Tooling/CommonOptionsParser.h"
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>

#include <FGenCompilationDatabase.hpp>
#include <FunctionGenerator.hpp>
#include <util/CommandLine.hpp>

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

static llvm::cl::opt<std::string> DatabasePath(
    "compilation-database",
    llvm::cl::desc(
        "Specify the compilation database <file>.\n"
        "Usually this <file> is named \"compile_commands.json\".\n"
        "If not specified fgen will automatically search all\n"
        "parent directories for such a file."
    ),
    llvm::cl::value_desc("file"),
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

int main(int argc, const char *argv[])
{
    std::unique_ptr<clang::tooling::CompilationDatabase> Database;
    std::string ErrMsg;

    clang::tooling::CommonOptionsParser Parser(argc, argv, FGenOptions);

    auto Files = Parser.getSourcePathList();
    if (Files.empty()) {
        util::cl::error() << "fgen: no source files specified - done.";
        std::exit(EXIT_FAILURE);
    }

    if (!DatabasePath.empty()) {
        Database = FGenCompilationDatabase::loadFromFile(DatabasePath, ErrMsg);

        if (!ErrMsg.empty()) {
            util::cl::error() << "fgen: failed to load provided compilation "
                              << "database \"" << DatabasePath << "\":\n    "
                              << ErrMsg << "\n";
            std::exit(EXIT_FAILURE);
        }
    }

    if (!Database) {
        llvm::StringRef File = Files[0];

        Database = FGenCompilationDatabase::autoDetectFromSource(File, ErrMsg);

        if (!Database || !ErrMsg.empty()) {
            util::cl::error() << "fgen: failed to load compilation database:\n"
                              << "    " << ErrMsg << "\n";
            std::exit(EXIT_FAILURE);
        }
    }

    clang::tooling::ClangTool Tool(*Database, Files);

    auto Factory =
        clang::tooling::newFrontendActionFactory<MethodGeneratorAction>();
    return Tool.run(Factory.get());
}
