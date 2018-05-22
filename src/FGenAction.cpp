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

#include <FGenAction.hpp>
#include <FGenVisitor.hpp>

class FGenASTConsumer : public clang::ASTConsumer {
public:
    FGenASTConsumer() = default;

    void setTargets(std::shared_ptr<std::unordered_set<std::string>> Targets);

    virtual void HandleTranslationUnit(clang::ASTContext &Context) override;

private:
    std::shared_ptr<std::unordered_set<std::string>> Targets_;
};

void FGenASTConsumer::setTargets(
    std::shared_ptr<std::unordered_set<std::string>> Targets)
{
    Targets_ = std::move(Targets);
}

void FGenASTConsumer::HandleTranslationUnit(clang::ASTContext &Context)
{
    auto Visitor = FGenVisitor();
    Visitor.setTargets(Targets_);

    Visitor.TraverseDecl(Context.getTranslationUnitDecl());
    Visitor.dump();
}

void FGenAction::setTargets(
    std::shared_ptr<std::unordered_set<std::string>> Targets)
{
    Targets_ = std::move(Targets);
}

std::unique_ptr<clang::ASTConsumer>
FGenAction::CreateASTConsumer(clang::CompilerInstance &CI, llvm::StringRef File)
{
    auto Consumer = llvm::make_unique<FGenASTConsumer>();
    Consumer->setTargets(Targets_);

    return Consumer;
}

FGenActionFactory::FGenActionFactory()
    : Targets_(std::make_shared<std::unordered_set<std::string>>())
{}

std::unordered_set<std::string> &FGenActionFactory::targets()
{
    return *Targets_;
}

const std::unordered_set<std::string> &FGenActionFactory::targets() const
{
    return *Targets_;
}

clang::FrontendAction *FGenActionFactory::create()
{
    auto Action = new FGenAction();
    Action->setTargets(Targets_);

    return Action;
}
