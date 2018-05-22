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

#ifndef FGEN_FGENACTION_HPP_
#define FGEN_FGENACTION_HPP_

#include <unordered_set>

#include <clang/Frontend/FrontendAction.h>
#include <clang/Tooling/Tooling.h>

#include <FGenConfiguration.hpp>

class FGenAction : public clang::ASTFrontendAction {
public:
    FGenAction() = default;

    void setConfiguration(std::shared_ptr<FGenConfiguration> Configuration);

    virtual std::unique_ptr<clang::ASTConsumer>
    CreateASTConsumer(clang::CompilerInstance &CI,
                      llvm::StringRef File) override;

private:
    std::shared_ptr<FGenConfiguration> Configuration_;
};

class FGenActionFactory : public clang::tooling::FrontendActionFactory {
public:
    FGenActionFactory();

    FGenConfiguration &configuration();
    const FGenConfiguration &configuration() const;

    virtual clang::FrontendAction *create() override;

private:
    std::shared_ptr<FGenConfiguration> Configuration_;
};

#endif /* FGEN_FGENACTION_HPP_ */
