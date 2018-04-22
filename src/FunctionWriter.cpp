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

#include <FunctionWriter.hpp>
#include <util/Decl.hpp>

FunctionWriter::FunctionWriter(std::string &Buffer)
    : OStream_(Buffer), IgnoreNamespaces_(true)
{
    OStream_.SetUnbuffered();
}

void FunctionWriter::write(const clang::FunctionDecl *FunctionDecl)
{
    writeTemplateParameters(FunctionDecl);
    writeReturnType(FunctionDecl);
    writeFullName(FunctionDecl);
    writeParameters(FunctionDecl);
    writeQualifiers(FunctionDecl);
    writeBody(FunctionDecl);
}

void FunctionWriter::writeTemplateParameters(
    const clang::FunctionDecl *FunctionDecl)
{
    llvm::SmallVector<const clang::DeclContext *, 8> DeclContextVec;

    util::decl::getDeclContexts(FunctionDecl, DeclContextVec);

    for (const auto DeclContext : DeclContextVec) {
        auto CXXRecordDecl = clang::dyn_cast<clang::CXXRecordDecl>(DeclContext);
        if (!CXXRecordDecl)
            continue;

        auto ClassTemplateDecl = CXXRecordDecl->getDescribedClassTemplate();
        if (!ClassTemplateDecl)
            continue;

        auto TemplateParams = ClassTemplateDecl->getTemplateParameters();
        writeTemplateParameters(TemplateParams);
    }

    auto FunctionTemplateDecl = FunctionDecl->getDescribedFunctionTemplate();
    if (!FunctionTemplateDecl)
        return;

    auto TemplateParams = FunctionTemplateDecl->getTemplateParameters();
    writeTemplateParameters(TemplateParams);
}

void FunctionWriter::writeTemplateParameters(
    const clang::TemplateParameterList *List)
{
    OStream_ << "template <";

    auto Begin = List->begin();
    auto End = List->end();

    for (auto It = Begin; It != End; ++It) {
        auto TTPDecl = clang::dyn_cast<clang::TemplateTypeParmDecl>(*It);
        auto NonTTPDecl = clang::dyn_cast<clang::NonTypeTemplateParmDecl>(*It);

        if (It != Begin)
            OStream_ << ", ";

        if (TTPDecl) {
            OStream_ << "typename ";

            if (TTPDecl->isParameterPack())
                OStream_ << "... ";

        } else if (NonTTPDecl) {
            auto Policy = NonTTPDecl->getASTContext().getPrintingPolicy();
            OStream_ << NonTTPDecl->getType().getAsString(Policy) << " ";
        }

        OStream_ << (*It)->getName();
    }

    OStream_ << ">\n";
}

void FunctionWriter::writeReturnType(const clang::FunctionDecl *FunctionDecl)
{
    if (clang::isa<clang::CXXConstructorDecl>(FunctionDecl))
        return;

    if (clang::isa<clang::CXXDestructorDecl>(FunctionDecl))
        return;

    auto QualType = FunctionDecl->getReturnType();
    auto &PrintingPolicy = FunctionDecl->getASTContext().getPrintingPolicy();

    QualType.print(OStream_, PrintingPolicy);

    /*
     * Output for reference and pointer types:
     *      "type &"
     *      "type *"
     * Output for other types:
     *      "type "
     */
    if (!QualType->isReferenceType() && !QualType->isPointerType())
        OStream_ << " ";
}

void FunctionWriter::writeFullName(
    const clang::FunctionDecl *const FunctionDecl)
{
    llvm::SmallVector<const clang::DeclContext *, 8> DeclContextVec;

    util::decl::getDeclContexts(FunctionDecl, DeclContextVec);

    /* Walk from to top declaration down to 'Decl' */

    for (const auto DeclContext : DeclContextVec) {
        if (IgnoreNamespaces_ && clang::isa<clang::NamespaceDecl>(DeclContext))
            continue;

        /*
         * Just writing the Constructor / Destructor Decl to the stream
         * does not work in case we are dealing with a
         * 'ClassTemplateDecl': it would result in something like (1)
         *      class<T>::class<T>()
         *                ^(1)
         * which does not compile.
         * We handle these cases here accordingly.
         */

        auto FunctionDecl = clang::dyn_cast<clang::FunctionDecl>(DeclContext);
        auto MethodDecl = clang::dyn_cast<clang::CXXMethodDecl>(DeclContext);
        auto NamedDecl = clang::dyn_cast<clang::NamedDecl>(DeclContext);

        if (clang::isa<clang::CXXConstructorDecl>(DeclContext))
            OStream_ << MethodDecl->getParent()->getName();
        else if (clang::isa<clang::CXXDestructorDecl>(DeclContext))
            OStream_ << '~' << MethodDecl->getParent()->getName();
        else if (FunctionDecl)
            OStream_ << *FunctionDecl;
        else
            OStream_ << NamedDecl->getName();

        /*
         * In a qualified name like
         *      namespace::class::method
         *
         * the 'class' declaration seems to be always a 'RecordDecl' regardless
         * wether it is templated or not. Here we explicitly check if it is a
         * template and if it is, we add the template arguments to
         * the qualified name.
         */
        auto RecordDecl = clang::dyn_cast<clang::RecordDecl>(NamedDecl);
        if (RecordDecl) {
            auto ClassTemplateDecl = RecordDecl->getDescribedTemplate();

            if (ClassTemplateDecl) {
                auto TParamList = ClassTemplateDecl->getTemplateParameters();

                OStream_ << "<";

                auto Begin = TParamList->begin();
                auto End = TParamList->end();

                for (auto It = Begin; It != End; ++It) {
                    if (It != Begin)
                        OStream_ << ", ";

                    OStream_ << (*It)->getName();
                }

                OStream_ << ">";
            }
        }

        if (DeclContext != DeclContextVec.back())
            OStream_ << "::";
    }
}

void FunctionWriter::writeParameters(const clang::FunctionDecl *FunctionDecl)
{
    auto &PrintingPolicy = FunctionDecl->getASTContext().getPrintingPolicy();
    unsigned int ArgCount = 0;

    OStream_ << "(";

    auto Begin = FunctionDecl->param_begin();
    auto End = FunctionDecl->param_end();

    for (auto It = Begin; It != End; ++It) {
        auto QualType = (*It)->getType();
        //         auto TypeName = QualType.getAsString(PrintingPolicy);

#if 0
        /* This also deletes qualifiers like 'const' */
        auto Index = TypeName.rfind("::");
        if (Index != std::string::npos) {
            /*
             * Given a type string like:
             *      std::vector::size_type
             * Reduce it to just:
             *      size_type
             *
             * The fully qualified function name makes sure that the compiler
             * will be able to resolve the correct type.
             */
            auto Begin = TypeName.begin();
            auto End = std::next(Begin, Index + std::strlen("::"));
            
            TypeName.erase(Begin, End);
        }
#endif

        if (It != Begin)
            OStream_ << ", ";

        QualType.print(OStream_, PrintingPolicy);
        //         OStream_ << TypeName;

        /*
         * Output for reference and pointer types:
         *      "type &"
         *      "type *"
         * Output for other types:
         *      "type "
         */

        if (!QualType->isReferenceType() && !QualType->isPointerType())
            OStream_ << " ";

        auto Name = (*It)->getName();
        if (Name.empty())
            OStream_ << "arg" << std::to_string(++ArgCount);
        else
            OStream_ << Name;
    }

    OStream_ << ")";
}

void FunctionWriter::writeQualifiers(const clang::FunctionDecl *FunctionDecl)
{
    auto MethodDecl = clang::dyn_cast<clang::CXXMethodDecl>(FunctionDecl);
    if (MethodDecl && MethodDecl->isConst())
        OStream_ << " const";

    OStream_ << "\n";
}

void FunctionWriter::writeBody(const clang::FunctionDecl *FunctionDecl)
{
    OStream_ << "{\n\n}\n\n";
}
