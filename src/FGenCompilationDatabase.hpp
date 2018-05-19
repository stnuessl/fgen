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

#ifndef FGEN_FGENCOMPILATIONDATABASE_HPP_
#define FGEN_FGENCOMPILATIONDATABASE_HPP_

#include <clang/Tooling/CompilationDatabase.h>

/*
 * A compilation database which transparently acts as a JSON compilation
 * database and a fixed compilation database at the same time.
 * If a compile command for a requested source file is found in the
 * JSON compilation database it will be the returned. Otherwise,
 * a fixed compile command will be used.
 *
 * This is helpful as "fgen" is intended to be
 * run mostly on header files. These files normally will not appear in
 * json compilation databases so we need a good fallback solution.
 */

class FGenCompilationDatabase : public clang::tooling::CompilationDatabase {
public:
    FGenCompilationDatabase(
        std::unique_ptr<clang::tooling::CompilationDatabase> JSONDatabase,
        std::unique_ptr<clang::tooling::CompilationDatabase> FixedDatabase);

    virtual std::vector<clang::tooling::CompileCommand>
    getCompileCommands(llvm::StringRef File) const override;

    virtual std::vector<std::string> getAllFiles() const override;

    virtual std::vector<clang::tooling::CompileCommand>
    getAllCompileCommands() const override;

    static std::unique_ptr<clang::tooling::CompilationDatabase>
    loadFromFile(llvm::StringRef File, std::string &ErrMsg);
    //
    //     static std::unique_ptr<clang::tooling::CompilationDatabase>
    //     loadFromDirectory(llvm::StringRef Directory, std::string &ErrMsg);
    //
    //     static std::unique_ptr<clang::tooling::CompilationDatabase>
    //     autoDetectFromDirectory(llvm::StringRef Directory, std::string
    //     &ErrMsg);

    static std::unique_ptr<clang::tooling::CompilationDatabase>
    autoDetectFromSource(llvm::StringRef SourceFile, std::string &ErrMsg);

private:
    std::unique_ptr<clang::tooling::CompilationDatabase> JSONDatabase_;
    std::unique_ptr<clang::tooling::CompilationDatabase> FixedDatabase_;
};

#endif /* FGEN_FGENCOMPILATIONDATABASE_HPP_ */
