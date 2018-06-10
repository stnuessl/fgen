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

#include <clang/Frontend/FrontendActions.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>

#include <util/CommandLine.hpp>

#include <FGenAction.hpp>
#include <FGenCompilationDatabase.hpp>
#include <FGenVisitor.hpp>

/* clang-format off */

static llvm::cl::OptionCategory GeneralOptions("1. General");
static llvm::cl::OptionCategory GeneratorOptions("2. Generator Options");

static llvm::cl::list<std::string> TargetVec(
    "ftarget",
    llvm::cl::desc(
        "Generate function bodys for <name>. <name> must be\n"
        "a valid prefix of a fully qualified identifier."
    ),
    llvm::cl::value_desc("name"),
    llvm::cl::CommaSeparated,
    llvm::cl::cat(GeneralOptions)
);

static llvm::cl::opt<bool> FlagAccessors(
    "faccessors",
    llvm::cl::desc(
        "Try to automatically implement accessor functions.\n"
    ),
    llvm::cl::cat(GeneratorOptions),
    llvm::cl::ValueOptional,
    llvm::cl::init(true)
);

static llvm::cl::opt<bool> FlagConversions(
    "fconversions",
    llvm::cl::desc(
        "Try to automatically implement C++ conversion operators."
    ),
    llvm::cl::cat(GeneratorOptions),
    llvm::cl::ValueOptional,
    llvm::cl::init(true)
);

static llvm::cl::opt<bool> FlagAllowMove(
    "fmove",
    llvm::cl::desc(
        "Automatically use the C++ move assignment if possible."
    ),
    llvm::cl::cat(GeneratorOptions),
    llvm::cl::ValueOptional,
    llvm::cl::init(true)
);

static llvm::cl::opt<bool> FlagStubs(
    "fstubs",
    llvm::cl::desc(
        "Try to automatically implement function stubs\n"
        "which are ready to be compiled."
    ),
    llvm::cl::cat(GeneratorOptions),
    llvm::cl::init(true)
);

static llvm::cl::opt<bool> FlagNamespaces(
    "fnamespaces",
    llvm::cl::desc(
        "Do not include namespaces in function qualifiers\n"
        "when generating function definitions."
    ),
    llvm::cl::cat(GeneratorOptions),
    llvm::cl::ValueOptional,                    
    llvm::cl::init(true)
);

static llvm::cl::opt<std::string> DatabasePath(
    "compilation-database",
    llvm::cl::desc(
        "Specifies the project's compilation database file.\n"
        "If no file is specified, fgen attempts to automatically\n"
        "find one."
    ),
    llvm::cl::value_desc("file"),
    llvm::cl::cat(GeneralOptions)
);

static llvm::cl::opt<std::string> OutputFile(
    "o",
    llvm::cl::desc(
        "Specifies the file to which the generated functions "
        "will be written to."
    ),
    llvm::cl::value_desc("file"),
    llvm::cl::cat(GeneralOptions)
);

static llvm::cl::list<std::string> InputFiles(
    llvm::cl::desc("[<file> ...]"),
    llvm::cl::Positional,
    llvm::cl::ZeroOrMore,
    llvm::cl::PositionalEatsArgs
);

/* clang-format on */

#define FGEN_VERSION_MAJOR "0"
#define FGEN_VERSION_MINOR "1"
#define FGEN_VERSION_PATCH "0"
#define FGEN_VERSION_INFO                                                      \
    FGEN_VERSION_MAJOR "." FGEN_VERSION_MINOR "." FGEN_VERSION_PATCH

#define FGEN_HOMEPAGE_URL "https://github.com/stnuessl/fgen"

#define FGEN_LICENSE_INFO                                                      \
    "License GPLv3+: GNU GPL version 3 or later "                              \
    "<http://www.gnu.org/licenses/gpl.html>\n"                                 \
    "This is free software: you are free to change and redistribute it.\n"     \
    "There is NO WARRANTY, to the extent permitted by law.\n"

int main(int argc, const char *argv[])
{
    std::unique_ptr<clang::tooling::CompilationDatabase> Database;
    std::string ErrMsg;

    llvm::cl::HideUnrelatedOptions({&GeneralOptions, &GeneratorOptions});

    const auto print_version = [](llvm::raw_ostream &OS) {
        OS << "fgen [v" << FGEN_VERSION_INFO << "] - " << FGEN_HOMEPAGE_URL
           << "\n"
           << FGEN_LICENSE_INFO << "\n";
    };

    llvm::cl::SetVersionPrinter(print_version);

    llvm::cl::ParseCommandLineOptions(argc, argv);

    /*
     * Seems like 'ParseCommandLineOptions' has to be called before
     * running this. Otherwise 'PrintHelpMessage' will cause a
     * segmentation fault.
     * Also, 'PrintHelpMessage' will terminate the program.
     */
    if (argc <= 1)
        llvm::cl::PrintHelpMessage(false, true);

    auto &Files = InputFiles;
    if (Files.empty()) {
        util::cl::error() << "fgen: no source files specified - done.\n";
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

    auto Factory = FGenActionFactory();
    auto &Configuration = Factory.configuration();

    auto Begin = std::make_move_iterator(TargetVec.begin());
    auto End = std::make_move_iterator(TargetVec.end());

    Configuration.setAllowMove(FlagAllowMove);
    Configuration.setImplementAccessors(FlagAccessors);
    Configuration.setImplemenConversions(FlagConversions);
    Configuration.setImplementStubs(FlagStubs);
    Configuration.setWriteNamespaces(FlagNamespaces);
    Configuration.setOutputFile(std::move(OutputFile));
    auto &Targets = Configuration.targets();
    Targets.insert(Targets.end(), Begin, End);

    clang::tooling::ClangTool Tool(*Database, Files);

    return Tool.run(&Factory);
}
