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

#include <cctype>

#include <llvm/ADT/StringExtras.h>

#include <FunctionWriter.hpp>
#include <util/Decl.hpp>

llvm::StringRef lastSection(llvm::StringRef S)
{
    S = S.rtrim('_');
    
    auto Index = S.rfind('_');
    if (Index != llvm::StringRef::npos)
        return S.substr(Index + 1);
    
    if (S.empty())
        return S;

    auto Begin = S.begin();
    auto End = S.end();
    auto LastUpper = End;
    auto It = End - 1;
    
    while (It >= Begin) {
        /* 
         * If we find an upper character we know that
         * on the next non-upper character we found the
         * relevant last section of the string.
         */
        if (std::isupper(*It))
            LastUpper = It;
        else if (LastUpper != End)
            return llvm::StringRef(LastUpper, End - LastUpper);
        
        --It;
    }
    
    return S;
}

// static unsigned int getLCSLength(llvm::StringRef S1, llvm::StringRef S2)
// {
//     unsigned int Max = 0;
//     
//     if (S1.size() < S2.size())
//         std::swap(S1, S2);
//     
//     for (size_t i = 0; i <  S1.size() && Max < S2.size() ; ++i) {
//         unsigned int Value = 0;
//         
//         for (size_t j = 0; i + j < S1.size() && j < S2.size() ; ++j)
//             Value += (llvm::toLower(S1[i + j]) == llvm::toLower(S2[j]));
// 
//         Max = std::max(Max, Value);
//     }
//     
//     return Max;
// }

static const clang::FieldDecl *
bestFieldDeclMatch(const clang::RecordDecl *RecordDecl, 
                   clang::QualType Type,
                   llvm::StringRef Name)
{
    if (RecordDecl->field_empty())
        return nullptr;

    auto MaxEditDistance = Name.size();
    
    Name = lastSection(Name);
    auto NameSize = static_cast<int>(Name.size());
    
    const clang::FieldDecl *BestMatch = nullptr;
    unsigned int BestEditDistance = MaxEditDistance;
    
    for (const auto &Field : RecordDecl->fields()) {
        if (Type != Field->getType())
            continue;
        
        auto FieldName = lastSection(Field->getName());
        auto FieldNameSize = static_cast<int>(FieldName.size());

        /* 
         * The "edit_distance" function below is O(m*n) where
         * m and n are the respective string sizes. We try to shortcut it
         * for names that cannot possibly be have a smaller edit distance
         * with this simple comparison.
         */
        if (std::abs(NameSize - FieldNameSize) > BestEditDistance)
            continue;
        
        auto Distance = Name.edit_distance(FieldName, true, BestEditDistance);
        
        if (Distance < BestEditDistance) {
            BestMatch = Field;
            BestEditDistance = Distance;
        }
    }
    
    return BestMatch;
}

FunctionWriter::FunctionWriter(std::string &Buffer)
    : OStream_(Buffer), IgnoreNamespaces_(true), ImplementAccessors_(true)
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

        if (It != Begin)
            OStream_ << ", ";

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
    if (ImplementAccessors_) {
        if (tryWriteGetAccessor(FunctionDecl))
            return;
        
        if (tryWriteSetAccessor(FunctionDecl))
            return;
    }
    
    OStream_ << "{\n\n}\n\n";
}

bool FunctionWriter::tryWriteGetAccessor(const clang::FunctionDecl *FunctionDecl)
{
    if (FunctionDecl->getReturnType()->isVoidType())
        return false;
    
    auto MethodDecl = clang::dyn_cast<clang::CXXMethodDecl>(FunctionDecl);
    if (MethodDecl)
        return tryWriteCXXGetAccessor(MethodDecl);
    
    return tryWriteCGetAccessor(FunctionDecl);
}


bool FunctionWriter::tryWriteCGetAccessor(const clang::FunctionDecl *FunctionDecl)
{
    auto Parameters = FunctionDecl->parameters();
    
    if (Parameters.size() != 1)
        return false;
    
    auto Parameter = Parameters[0];
    auto Type = Parameter->getType();
    
    if (!Type->isPointerType())
        return false;
    
    auto RecordDecl = Type->getPointeeCXXRecordDecl();
    if (!RecordDecl)
        return false;
    
    auto ReturnType = FunctionDecl->getReturnType();
    auto Name = FunctionDecl->getName();
    
    auto FieldDecl = bestFieldDeclMatch(RecordDecl, ReturnType, Name);
    if (!FieldDecl)
        return false;
    
    auto ParameterName = Parameter->getName();
    
    OStream_ << "{\n    return ";
    
    if (ParameterName.empty())
        OStream_ << "arg1";
    else
        OStream_ << ParameterName;
    
    OStream_ << "->" << FieldDecl->getName() << ";\n}\n\n";
    
    return true;
}

bool FunctionWriter::tryWriteCXXGetAccessor(const clang::CXXMethodDecl *MethodDecl)
{
    if (!MethodDecl->isConst())
        return false;
    
    if (!MethodDecl->parameters().empty())
        return false;
    
    auto RecordDecl = MethodDecl->getParent();
    
    auto ReturnType = MethodDecl->getReturnType();
    auto Name = MethodDecl->getName();
    
    auto FieldDecl = bestFieldDeclMatch(RecordDecl, ReturnType, Name);
    if (!FieldDecl)
        return false;
    
    OStream_ << "{\n    return " << FieldDecl->getName() << ";\n}\n\n";
    
    return true;
}


bool FunctionWriter::tryWriteCSetAccessor(const clang::FunctionDecl *FunctionDecl)
{
    auto Parameters = FunctionDecl->parameters();
    
    if (Parameters.size() != 2)
        return false;
    
    auto Parameter1 = Parameters[0];
    auto Parameter2 = Parameters[1];
        
    auto Parameter1Type = Parameter1->getType();
    
    auto PointerType = clang::dyn_cast<clang::PointerType>(Parameter1Type);
    if (!PointerType || PointerType->getPointeeType().isConstQualified())
        return false;

    auto RecordDecl = Parameter1Type->getPointeeCXXRecordDecl();
    if (!RecordDecl)
        return false;
    
    auto Parameter2Type = Parameter2->getType();
    auto Name = FunctionDecl->getName();
    
    auto FieldDecl = bestFieldDeclMatch(RecordDecl, Parameter2Type, Name);
    if (!FieldDecl)
        return false;
    
    int ArgCount = 0;
    auto Name1 = Parameter1->getName();
    auto Name2 = Parameter2->getName();
    
    OStream_ << "{\n    ";
    
    if (Name1.empty())
        OStream_ << "arg" << std::to_string(++ArgCount);
    else
        OStream_ << Name1;
    
    OStream_ << "->" << FieldDecl->getName() << " = ";

    if (Name2.empty())
        OStream_ << "arg" << std::to_string(++ArgCount);
    else
        OStream_ << Name2;

    OStream_ << ";\n}\n\n";
    
    return true;
}

bool FunctionWriter::tryWriteCXXSetAccessor(const clang::CXXMethodDecl *MethodDecl)
{
    if (MethodDecl->isConst())
        return false;
    
    auto Parameters = MethodDecl->parameters();
    
    if (Parameters.size() != 1)
        return false;
    
    auto RecordDecl = MethodDecl->getParent();
    
    auto Type = Parameters[0]->getType();
    auto Name = MethodDecl->getName();
    
    auto FieldDecl = bestFieldDeclMatch(RecordDecl, Type, Name);
    if (!FieldDecl)
        return false;
    
    OStream_ << "{\n    " << FieldDecl->getName() << " = ";
    
    auto ArgName = Parameters[0]->getName();
    if (ArgName.empty())
        OStream_ << "arg1";
    else
        OStream_ << ArgName;
    
    OStream_ << ";\n}\n\n";
    
    return true;
}

bool FunctionWriter::tryWriteSetAccessor(const clang::FunctionDecl *FunctionDecl)
{
    if (!FunctionDecl->getReturnType()->isVoidType())
        return false;
    
    auto MethodDecl = clang::dyn_cast<clang::CXXMethodDecl>(FunctionDecl);
    if (MethodDecl)
        return tryWriteCXXSetAccessor(MethodDecl);
    
    return tryWriteCSetAccessor(FunctionDecl);
}
