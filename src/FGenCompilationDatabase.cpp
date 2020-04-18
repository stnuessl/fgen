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

//#include <llvm/ADT/SmallString.h>
#include <llvm/Support/FileSystem.h>
//#include <llvm/Support/Path.h>
#include <clang/Tooling/JSONCompilationDatabase.h>

#include <FGenCompilationDatabase.hpp>

bool FGenCompilationDatabase::autoDetect(llvm::StringRef Path, 
                                         std::string &ErrMsg)
{
    using namespace clang::tooling;

    std::unique_ptr<clang::tooling::CompilationDatabase> Database; 
    bool Ok;

    /* Follow symbolic links */
    switch (llvm::sys::fs::get_file_type(Path, true)) {
    case llvm::sys::fs::file_type::regular_file:
        /*
         * Try to detect the type of compilation database to load.
         * If we are unable to to do this, we recursively search
         * the parent directories of 'Path'.
         */

        if (Path.endswith_lower(".json")) {
            auto Arg = JSONCommandLineSyntax::AutoDetect;

            Database = JSONCompilationDatabase::loadFromFile(Path, ErrMsg, Arg);
            break;
        }
       
        if (Path.endswith_lower(".txt")) {
            Database = FixedCompilationDatabase::loadFromFile(Path, ErrMsg);
            break;
        }

        Database = CompilationDatabase::autoDetectFromSource(Path, ErrMsg);

        break;
    case llvm::sys::fs::file_type::directory_file:
        Database = CompilationDatabase::autoDetectFromDirectory(Path, ErrMsg);
        break;
    case llvm::sys::fs::file_type::status_error:
        if (!llvm::sys::fs::exists(Path)) {
            ErrMsg += "no such file or directory";
            break;
        }

        ErrMsg += "failed to retrieve file status";
        break;
    default:
        ErrMsg += "invalid file type";
        break;
    }

    Ok = (Database != nullptr);
    
    /* Transer ownership of found compilation database */
    if (Ok)
        Database_ = std::move(Database);

    return Ok;
}

const clang::tooling::CompilationDatabase &FGenCompilationDatabase::get() const
{
    return *Database_;
}


