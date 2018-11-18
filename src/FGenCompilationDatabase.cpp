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

#include <llvm/ADT/SmallString.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>

#include <clang/Tooling/JSONCompilationDatabase.h>

#include <FGenCompilationDatabase.hpp>

llvm::StringRef getDefaultClangIncludeDirectory()
{
    static const char Data[] = "/usr/lib/clang/" LLVM_VERSION_STRING "/include";

    return llvm::StringRef(Data, sizeof(Data) - 1);
}

static std::unique_ptr<clang::tooling::CompilationDatabase>
createBasicFixedCompilationDatabase(llvm::StringRef CompilationDirectory)
{
    llvm::SmallString<128> Buffer;
    /* Create a simplistic fixed compilation database out of thin air */
    std::vector<std::string> CommandLine = { };

    /*
     * A common project layout is
     *      ./project
     *      ./project/src
     *      ./project/include
     *      ...
     *
     * Try to add such directories to the header search path.
     * Ofcourse this is really risky and will probably only work
     * rarely, but at this point the situation is already bad.
     */

    llvm::Twine SourceDirectory(CompilationDirectory, "/src");
    if (llvm::sys::fs::is_directory(SourceDirectory)) {
        Buffer = "-I";
        SourceDirectory.toVector(Buffer);
        CommandLine.push_back(Buffer.str());
    }

    llvm::Twine IncludeDirectory(CompilationDirectory, "/include");
    if (llvm::sys::fs::is_directory(IncludeDirectory)) {
        Buffer = "-I";
        IncludeDirectory.toVector(Buffer);
        CommandLine.push_back(Buffer.str());
    }

    return llvm::make_unique<clang::tooling::FixedCompilationDatabase>(
        CompilationDirectory, CommandLine);
}

static std::unique_ptr<clang::tooling::CompilationDatabase>
loadJSONDatabase(llvm::StringRef File, std::string &ErrMsg)
{
    auto Syntax = clang::tooling::JSONCommandLineSyntax::AutoDetect;

    return clang::tooling::JSONCompilationDatabase::loadFromFile(File, ErrMsg,
                                                                 Syntax);
}

static std::unique_ptr<clang::tooling::CompilationDatabase>
loadJSONDatabase(llvm::Twine &File, std::string &ErrMsg)
{
    llvm::SmallString<256> Buffer;

    File.toVector(Buffer);

    return loadJSONDatabase(Buffer, ErrMsg);
}

static std::unique_ptr<clang::tooling::CompilationDatabase>
loadFixedDatabase(llvm::StringRef File, std::string &ErrMsg)
{
    llvm::SmallString<256> Buffer;

    /*
     * Sanitize 'File': The "FixedCompilationDatabase" will assume
     * the compilation directory is "llvm::sys::path::parent_path(File)".
     * This is an issue if "File" is just a file name,
     * e.g. "compile_flags.txt" results in the directory "".
     */
    auto Error = llvm::sys::fs::real_path(File, Buffer, true);
    if (Error) {
        ErrMsg += Error.message();
        return nullptr;
    }

    File = Buffer.str();

    return clang::tooling::FixedCompilationDatabase::loadFromFile(File, ErrMsg);
}

static std::unique_ptr<clang::tooling::CompilationDatabase>
loadFixedDatabase(llvm::Twine &File, std::string &ErrMsg)
{
    llvm::SmallString<256> Buffer;

    File.toVector(Buffer);

    return clang::tooling::FixedCompilationDatabase::loadFromFile(Buffer,
                                                                  ErrMsg);
}

static void
addIncludeDirectory(std::vector<clang::tooling::CompileCommand> &Commands,
                    llvm::StringRef Directory)
{
    llvm::SmallString<256> Buffer;
    llvm::Twine Twine("-I", Directory);

    llvm::StringRef NewArgument = Twine.toStringRef(Buffer);

    auto equalsNewArgument = [NewArgument](const std::string &Argument) {
        return NewArgument.equals(Argument);
    };

    for (auto &Command : Commands) {
        if (llvm::none_of(Command.CommandLine, equalsNewArgument))
            Command.CommandLine.push_back(NewArgument);
    }
}

FGenCompilationDatabase::FGenCompilationDatabase(
    std::unique_ptr<clang::tooling::CompilationDatabase> JSONDatabase,
    std::unique_ptr<clang::tooling::CompilationDatabase> FixedDatabase)
    : JSONDatabase_(std::move(JSONDatabase)),
      FixedDatabase_(std::move(FixedDatabase))
{
    if (FixedDatabase_)
        return;

    /*
     * We need to always have the fixed compilation database as a
     * fallback option. Try to create one from the JSON compilation database.
     * If this does not work we use a very basic fallback fixed compilation
     * database.
     */

    if (!JSONDatabase_) {
        FixedDatabase_ = createBasicFixedCompilationDatabase(".");
        return;
    }

    auto CompileCommands = JSONDatabase_->getAllCompileCommands();
    if (CompileCommands.empty()) {
        FixedDatabase_ = createBasicFixedCompilationDatabase(".");
        return;
    }

    /*
     * Search the compile commands for an compile command with an
     * "-c" flag, as such a command should have all the information needed
     * to parse the source files. If we cannot find such a compile
     * command we use the first one as a fallback.
     *
     * Obviously, it could be that some source files
     * need different compilation flags, but we cannot know that anyway
     * until actually parsing them. We do not know if this will
     * be the correct one, or if there even is a correct one, but this
     * will be our best guess.
     */
    auto CCBegin = CompileCommands.begin();
    auto CCEnd = CompileCommands.end();

    auto IsCompileCommand = [](const clang::tooling::CompileCommand &Command) {
        auto Pred = [](const std::string &Arg) { return Arg == "-c"; };

        return llvm::any_of(Command.CommandLine, Pred);
    };

    auto It = std::find_if(CCBegin, CCEnd, IsCompileCommand);
    if (It == CCEnd)
        It = CCBegin;

    /*
     * Take the first entry of the JSON compilation database and
     * use its compile command as the basis for the fixed compilation
     * database.
     */
    auto &Filename = It->Filename;
    auto &Directory = It->Directory;
    auto &CommandLine = It->CommandLine;

    /*
     * Remove the invoked program from the command as
     * the fixed compilation database will prepend "clang-tool"
     * to it anyway.
     */
    if (!CommandLine.empty())
        CommandLine.erase(CommandLine.begin());

    /*
     * Removed the compilation unit's file name from the
     * command as the fixed compilation database will append
     * each requested file automatically to it.
     */
    auto Begin = CommandLine.begin();
    auto End = CommandLine.end();

    auto NewEnd = std::remove(Begin, End, Filename);
    CommandLine.erase(NewEnd, End);

    /* Just a temporary variable to keep the code formatting clean */
    auto DB = llvm::make_unique<clang::tooling::FixedCompilationDatabase>(
        std::move(Directory), std::move(CommandLine));

    FixedDatabase_ = std::move(DB);
}

std::vector<clang::tooling::CompileCommand>
FGenCompilationDatabase::getCompileCommands(llvm::StringRef File) const
{
    std::vector<clang::tooling::CompileCommand> Result;

    if (JSONDatabase_)
        Result = JSONDatabase_->getCompileCommands(File);

    if (Result.empty())
        Result = FixedDatabase_->getCompileCommands(File);

    auto Directory = getDefaultClangIncludeDirectory();

    addIncludeDirectory(Result, Directory);

    return Result;
}

std::vector<std::string> FGenCompilationDatabase::getAllFiles() const
{
    if (JSONDatabase_)
        return JSONDatabase_->getAllFiles();

    return std::vector<std::string>();
}

std::vector<clang::tooling::CompileCommand>
FGenCompilationDatabase::getAllCompileCommands() const
{
    std::vector<clang::tooling::CompileCommand> Result;

    if (JSONDatabase_) {
        Result = JSONDatabase_->getAllCompileCommands();

        auto Directory = getDefaultClangIncludeDirectory();
        addIncludeDirectory(Result, Directory);
    }

    return Result;
}

std::unique_ptr<clang::tooling::CompilationDatabase>
FGenCompilationDatabase::loadFromFile(llvm::StringRef File, std::string &ErrMsg)
{
    bool isRegular;

    ErrMsg.clear();

    llvm::raw_string_ostream OS(ErrMsg);
    OS.SetUnbuffered();

    auto Error = llvm::sys::fs::is_regular_file(File, isRegular);
    if (Error) {
        OS << "failed to load compilation database \"" << File << "\" - "
           << Error.message();

        return nullptr;
    }

    if (!isRegular) {
        OS << "failed to load compilation database \"" << File << "\" - "
           << "the file is not a regular file.";

        return nullptr;
    }

    auto JDB = loadJSONDatabase(File, ErrMsg);
    if (JDB) {
        return llvm::make_unique<FGenCompilationDatabase>(std::move(JDB),
                                                          nullptr);
    }

    auto FDB = loadFixedDatabase(File, ErrMsg);
    if (FDB) {
        return llvm::make_unique<FGenCompilationDatabase>(nullptr,
                                                          std::move(FDB));
    }

    ErrMsg.clear();

    OS << "failed to load compilation database \"" << File << "\" - "
       << "invalid file format. File is neither a valid "
       << "\"compile_commands.json\" nor a valid \"compile_flags.txt\"";

    return nullptr;
}

std::unique_ptr<clang::tooling::CompilationDatabase>
FGenCompilationDatabase::autoDetectFromSource(llvm::StringRef SourceFile,
                                              std::string &ErrMsg)
{
    std::unique_ptr<clang::tooling::CompilationDatabase> JSONDatabase;
    std::unique_ptr<clang::tooling::CompilationDatabase> FixedDatabase;
    llvm::SmallString<256> Buffer;

    ErrMsg.clear();

    /*
     * Sanitize 'SourceFile' input to make traversing up to the root
     * directory straighforward.
     */
    auto Error = llvm::sys::fs::real_path(SourceFile, Buffer, true);
    if (Error) {
        ErrMsg = Error.message();
        return nullptr;
    }

    SourceFile = Buffer.str();
    llvm::StringRef Directory = llvm::sys::path::parent_path(SourceFile);

    while (!Directory.empty() && !(JSONDatabase && FixedDatabase)) {
        /*
         * A non empty "ErrMsg" indicates that there was a valid
         * "compile_commands.json" or "compile_flags.txt" file
         * to load but it failed. We will notify the user about
         * this and exit the function returning a null pointer.
         */

        llvm::Twine JSONFile(Directory, "/compile_commands.json");
        if (!JSONDatabase && llvm::sys::fs::is_regular_file(JSONFile)) {
            JSONDatabase = loadJSONDatabase(JSONFile, ErrMsg);

            if (!ErrMsg.empty())
                return nullptr;
        }

        llvm::Twine FixedFile(Directory, "/compile_flags.txt");
        if (!FixedDatabase && llvm::sys::fs::is_regular_file(FixedFile)) {
            FixedDatabase = loadFixedDatabase(FixedFile, ErrMsg);

            if (!ErrMsg.empty())
                return nullptr;
        }

        Directory = llvm::sys::path::parent_path(Directory);
    }

    return llvm::make_unique<FGenCompilationDatabase>(std::move(JSONDatabase),
                                                      std::move(FixedDatabase));
}
