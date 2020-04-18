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

class FGenCompilationDatabase {
public:
    FGenCompilationDatabase() = default;

    bool autoDetect(llvm::StringRef Path, std::string &ErrMsg);

    const clang::tooling::CompilationDatabase &get() const;

private:
    std::unique_ptr<clang::tooling::CompilationDatabase> Database_;
};

#endif /* FGEN_FGENCOMPILATIONDATABASE_HPP_ */
