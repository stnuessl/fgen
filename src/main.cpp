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
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>

#include <FunctionGenerator.hpp>
#include <util/CompilationDatabase.hpp>

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
    using namespace clang::tooling;

    std::unique_ptr<clang::tooling::CompilationDatabase> Database;
    std::string ErrMsg;
    llvm::SmallString<128> Buffer;

    /* Get th current working directory */
    llvm::sys::fs::current_path(Buffer);
    auto CurrentPath = Buffer.str();

    CommonOptionsParser Parser(argc, argv, FGenOptions);

    auto &Files = Parser.getSourcePathList();

    /*
     * Find a hopefully working compilation database (compile_commands.json).
     */
    if (!DatabasePath.empty()) {
        Database = util::compilation_database::load(DatabasePath, ErrMsg);

        if (!Database) {
            llvm::errs() << "** Error: fgen: failed to load provided "
                         << "compilation database \"" << DatabasePath << "\" - "
                         << ErrMsg << "\n";
            std::exit(EXIT_FAILURE);
        }
    } else {
        Database = util::compilation_database::detect(CurrentPath, ErrMsg);

        if (!Database && !ErrMsg.empty()) {
            llvm::errs() << "** ERROR: fgen: failed to load detected "
                         << "compilation database - " << ErrMsg << "\n";
            std::exit(EXIT_FAILURE);
        }
    }

    /*
     * What good is a compilation database if it does not contain a command
     * for parsing the user requested source files?
     * However, this is just a very basic fallback database, which will not
     * work for any translation unit with non default include paths.
     */
    if (!Database || !util::compilation_database::contains(*Database, Files))
        Database = util::compilation_database::make(CurrentPath, Files, ErrMsg);

    ClangTool Tool(*Database, Files);

    return Tool.run(newFrontendActionFactory<MethodGeneratorAction>().get());
}
