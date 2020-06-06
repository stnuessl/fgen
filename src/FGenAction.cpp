/*
 * Copyright (C) 2018  Steffen Nüssle
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

#include <llvm/Support/raw_ostream.h>

#include <util/CommandLine.hpp>

#include <FGenAction.hpp>
#include <FGenVisitor.hpp>

class FGenASTConsumer : public clang::ASTConsumer {
public:
    FGenASTConsumer() = default;

    void setConfiguration(std::shared_ptr<FGenConfiguration> Configuration);

    virtual void HandleTranslationUnit(clang::ASTContext &Context) override;

private:
    std::shared_ptr<FGenConfiguration> Configuration_;
};

void FGenASTConsumer::setConfiguration(
    std::shared_ptr<FGenConfiguration> Configuration)
{
    Configuration_ = std::move(Configuration);
}

void FGenASTConsumer::HandleTranslationUnit(clang::ASTContext &Context)
{
    auto Visitor = FGenVisitor();

    Visitor.setConfiguration(Configuration_);
    Visitor.TraverseDecl(Context.getTranslationUnitDecl());

    auto &OutputFile = Configuration_->outputFile();

    if (!OutputFile.empty()) {
        std::error_code Error;

        llvm::raw_fd_ostream OS(OutputFile, Error, llvm::sys::fs::F_Append);
        if (Error) {
            util::cl::error() << "fgen: failed to open file \"" << OutputFile
                              << "\" for writing:\n"
                              << "    " << Error.message() << "\n";
            std::exit(EXIT_FAILURE);
        }

        Visitor.dump(OS);
        return;
    }

    Visitor.dump(llvm::outs());
}

void FGenAction::setConfiguration(
    std::shared_ptr<FGenConfiguration> Configuration)
{
    Configuration_ = std::move(Configuration);
}

std::unique_ptr<clang::ASTConsumer>
FGenAction::CreateASTConsumer(clang::CompilerInstance &CI, llvm::StringRef File)
{
    (void) CI;
    (void) File;

    auto Consumer = llvm::make_unique<FGenASTConsumer>();
    Consumer->setConfiguration(Configuration_);

    return Consumer;
}

FGenActionFactory::FGenActionFactory()
    : Configuration_(std::make_shared<FGenConfiguration>())
{
    /* clang-format... */
}

FGenConfiguration &FGenActionFactory::configuration()
{
    return *Configuration_;
}
const FGenConfiguration &FGenActionFactory::configuration() const
{
    return *Configuration_;
}

clang::FrontendAction *FGenActionFactory::create()
{
    auto Action = new FGenAction();
    Action->setConfiguration(Configuration_);

    return Action;
}
