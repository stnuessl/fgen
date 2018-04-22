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

#include "llvm/Support/Path.h"
#include <clang/Tooling/JSONCompilationDatabase.h>
#include <llvm/Support/raw_ostream.h>

#include <util/CompilationDatabase.hpp>

namespace {

std::string generateCompilationDatabase(llvm::ArrayRef<std::string> Files,
                                        llvm::StringRef WorkingDirectory)
{
    std::string CompilationDatabaseBuffer;
    CompilationDatabaseBuffer.reserve(2048);

    llvm::raw_string_ostream OStream(CompilationDatabaseBuffer);
    OStream.SetUnbuffered();

    auto ClangDirectory = "/usr/lib/clang/" LLVM_VERSION_STRING "/include";
    auto Compiler = "/usr/bin/clang++";

    OStream << "[\n";

    for (const auto &File : Files) {
        OStream << "    {\n"
                << "        \"file\": \"" << File << "\",\n"
                << "        \"command\": \"" << Compiler << " -I"
                << ClangDirectory << " " << WorkingDirectory << "/" << File
                << "\",\n"
                << "        \"directory\": \"" << WorkingDirectory << "\",\n"
                << "    },\n";
    }

    OStream << "]\n";

    return std::move(OStream.str());
}

} // namespace

namespace util {
namespace compilation_database {

std::unique_ptr<clang::tooling::CompilationDatabase> load(llvm::StringRef Path,
                                                          std::string &ErrMsg)
{
    using namespace clang::tooling;

    auto JSONSyntax = JSONCommandLineSyntax::AutoDetect;
    return JSONCompilationDatabase::loadFromFile(Path, ErrMsg, JSONSyntax);
}

std::unique_ptr<clang::tooling::CompilationDatabase>
detect(llvm::StringRef Directory, std::string &ErrMsg)
{
    using namespace clang::tooling;

    std::unique_ptr<clang::tooling::CompilationDatabase> Database;

    while (!Directory.empty()) {
        Database = CompilationDatabase::loadFromDirectory(Directory, ErrMsg);
        if (Database)
            return Database;

        Directory = llvm::sys::path::parent_path(Directory);
    }

    /*
     * An empty error message signals
     * that we could not find a compilation database.
     * Clang does not provide such an interface.
     */
    ErrMsg.clear();
    return Database;
    //     return CompilationDatabase::autoDetectFromDirectory(Directory,
    //     ErrMsg);
}

std::unique_ptr<clang::tooling::CompilationDatabase>
make(llvm::StringRef WorkingDirectory,
     llvm::ArrayRef<std::string> Files,
     std::string &ErrMsg)
{
    using namespace clang::tooling;

    auto JSONSyntax = JSONCommandLineSyntax::AutoDetect;
    auto Data = generateCompilationDatabase(Files, WorkingDirectory);

    llvm::outs() << "Created the following compilation database: " << Data
                 << "\n";

    return JSONCompilationDatabase::loadFromBuffer(Data, ErrMsg, JSONSyntax);
}

bool contains(const clang::tooling::CompilationDatabase &Database,
              llvm::ArrayRef<std::string> Files)
{
    auto DBFiles = Database.getAllFiles();

    auto Begin = DBFiles.begin();
    auto End = DBFiles.end();

    for (const auto &File : Files) {
        if (std::find(Begin, End, File) == End)
            return false;
    }

    return true;
}

} // namespace compilation_database
} // namespace util
