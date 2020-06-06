/*
 * Copyright (C) 2017  Steffen Nüssle
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

#include <FGenCompilationDatabase.hpp>
#include <FGenAction.hpp>
#include <FGenVisitor.hpp>

/* clang-format off */

static llvm::cl::OptionCategory GeneralOptions("1. General");
static llvm::cl::OptionCategory GeneratorOptions("2. Generator Options");

static llvm::cl::list<std::string> TargetVec(
    "fcontains",
    llvm::cl::desc(
        "Generate function bodys for <name>. <name> must be\n"
        "a valid substring of a fully qualified identifier."
    ),
    llvm::cl::value_desc("name"),
    llvm::cl::CommaSeparated,
    llvm::cl::cat(GeneralOptions)
);

static llvm::cl::opt<bool> FlagAccessors(
    "faccessors",
    llvm::cl::desc(
        "Try to automatically implement accessor functions."
    ),
    llvm::cl::cat(GeneratorOptions),
    llvm::cl::ValueOptional,
    llvm::cl::init(true)
);

static llvm::cl::opt<bool> FlagTrimOutput(
    "ftrim",
    llvm::cl::desc(
        "Remove unnecessary empty lines from the generated\n"
        "output. Such lines are intentionally added to improve\n"
        "the visual appearance of the code."
    ),
    llvm::cl::cat(GeneratorOptions),
    llvm::cl::ValueOptional,
    llvm::cl::init(false)
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
        "Try to automatically implement function stubs."
    ),
    llvm::cl::cat(GeneratorOptions),
    llvm::cl::init(true)
);

static llvm::cl::opt<bool> FlagNamespaces(
    "fnamespace-definitions",
    llvm::cl::desc(
        "Use namespace definitions instead of writing the\n"
        "namespace names in the function qualifiers."
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
    FGenCompilationDatabase FGenDb;
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
     */
    if (argc <= 1) {
        /* Show no hidden options, show categorized options */
        llvm::cl::PrintHelpMessage(false, true);
        std::exit(EXIT_FAILURE);
    }

    auto &Files = InputFiles;
    if (Files.empty()) {
        util::cl::error() << "fgen: no source files specified - done.\n";
        std::exit(EXIT_FAILURE);
    }

    if (!DatabasePath.empty()) {
        bool Ok = FGenDb.autoDetect(DatabasePath, ErrMsg);
        if (!Ok) {
            util::cl::error() << "fgen: failed to load compilation database \"" 
                              << DatabasePath << "\" - " << ErrMsg << "\n";
            std::exit(EXIT_FAILURE);
        }
    } else {
        /* Use user provided source file for auto detection */
        bool Ok = FGenDb.autoDetect(Files[0], ErrMsg);
        if (!Ok) {
            util::cl::error() << "fgen: failed to find and load a compilation "
                              << "database - " << ErrMsg << "\n";
            std::exit(EXIT_FAILURE);
        }
    }

    auto Factory = FGenActionFactory();
    auto &Configuration = Factory.configuration();

    auto Begin = std::make_move_iterator(TargetVec.begin());
    auto End = std::make_move_iterator(TargetVec.end());

    Configuration.setAllowMove(FlagAllowMove);
    Configuration.setTrimOutput(FlagTrimOutput);
    Configuration.setImplementAccessors(FlagAccessors);
    Configuration.setImplemenConversions(FlagConversions);
    Configuration.setImplementStubs(FlagStubs);
    Configuration.setNamespaceDefinitions(FlagNamespaces);
    Configuration.setOutputFile(std::move(OutputFile));

    auto &Targets = Configuration.targets();
    Targets.insert(Targets.end(), Begin, End);

    clang::tooling::ClangTool Tool(FGenDb.get(), Files);

    return Tool.run(&Factory);
}
