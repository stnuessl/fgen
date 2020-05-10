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

#include <FunctionGenerator.hpp>
#include <util/Decl.hpp>
#include <util/Type.hpp>

static llvm::StringRef extractRelevantSection(llvm::StringRef Name)
{
    /*
     * This functions tries to extract the last section of a
     * declaration or value name. Some examples:
     *
     * Input:                   Output:
     * "var_name_"      --->    "name"
     * "var_name"       --->    "name"
     * "FuncName_"      --->    "Name"
     * "Func_Name_"     --->    "Name"
     * "MACRO_NAME"     --->    "NAME"
     */
    auto TrimmedName = Name.rtrim('_');

    if (TrimmedName.empty())
        return Name;

    auto Index = TrimmedName.rfind('_');
    if (Index != llvm::StringRef::npos)
        return TrimmedName.substr(Index + 1);

    auto Begin = TrimmedName.begin();
    auto End = TrimmedName.end();
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

    return TrimmedName;
}

template <typename Pred>
static const clang::FieldDecl *bestFieldDeclMatch(
    const clang::RecordDecl *RecordDecl, llvm::StringRef Name, Pred &&PredFunc)
{
    /*
     * This function tries to find a field inside of "RecordDecl" which
     * matches fairly decently with "Name" and fullfills the type
     * predicate defined in "PredFunc".
     */
    if (RecordDecl->field_empty())
        return nullptr;

    if (Name.startswith_lower("set")) {
        auto Range = RecordDecl->fields();

        if (std::distance(Range.begin(), Range.end()) == 1) {
            auto FieldType = (*Range.begin())->getType();

            if (util::decl::isSingleBit(*Range.begin()))
                FieldType = (*Range.begin())->getASTContext().BoolTy;

            if (PredFunc(FieldType))
                return *Range.begin();
            else
                return nullptr;
        }
    }

    Name = extractRelevantSection(Name);
    auto NameSize = Name.size();

    const clang::FieldDecl *BestMatch = nullptr;
    auto BestEditDistance = NameSize;

    for (const auto &FieldDecl : RecordDecl->fields()) {
        auto FieldType = FieldDecl->getType();

        if (util::decl::isSingleBit(FieldDecl))
            FieldType = FieldDecl->getASTContext().BoolTy;

        if (!PredFunc(FieldType))
            continue;

        auto FieldName = extractRelevantSection(FieldDecl->getName());
        auto FieldNameSize = static_cast<int>(FieldName.size());

        /*
         * The "edit_distance" function below is O(m*n) where
         * m and n are the respective string sizes. We try to shortcut it
         * for names that cannot possibly have a smaller edit distance
         * with this comparison.
         */
        auto Diff = static_cast<int>(NameSize - FieldNameSize);
        
        if (std::abs(Diff) > static_cast<int>(BestEditDistance))
            continue;

        auto Distance = Name.edit_distance(FieldName, true, BestEditDistance);

        if (Distance < BestEditDistance) {
            BestMatch = FieldDecl;
            BestEditDistance = Distance;
        }
    }

    return BestMatch;
}

template<typename Pred>
static const clang::FieldDecl *
bestFieldDeclMatch(const clang::RecordDecl *RecordDecl, Pred &&PredFunc)
{
    /*
     * If there is exactly one field declaration in "RecordDecl" which
     * fullfills the type predicate, we will return it.
     */
    const clang::FieldDecl *Match = nullptr;

    for (const auto &FieldDecl : RecordDecl->fields()) {
        auto FieldType = FieldDecl->getType();

        if (util::decl::isSingleBit(FieldDecl))
            FieldType = FieldDecl->getASTContext().BoolTy;

        if (PredFunc(FieldType)) {
            if (Match)
                return nullptr;

            Match = FieldDecl;
        }
    }

    return Match;
}

FunctionGenerator::FunctionGenerator()
    : ActiveNamespaces_(),
      Includes_(),
      StrStream_(true),
      Configuration_(nullptr)
{}

void FunctionGenerator::setConfiguration(
    std::shared_ptr<FGenConfiguration> Configuration)
{
    Configuration_ = std::move(Configuration);
}

void FunctionGenerator::add(const clang::FunctionDecl *FunctionDecl)
{
    llvm::SmallVector<const clang::DeclContext *, 8> ContextVec;

    util::decl::getFullContext(FunctionDecl, ContextVec);
    
    writeNamespaceDefinitions(ContextVec);
    writeTemplateParameters(FunctionDecl);

    if (util::decl::hasTrailingReturnType(FunctionDecl)) {
        writeTrailingFunctionStart();
        writeFullQualifiedName(ContextVec);
        writeParameters(FunctionDecl);
        writeQualifiers(FunctionDecl);
        writeTrailingReturnType(FunctionDecl);
    } else {
        writeReturnType(FunctionDecl);
        writeFullQualifiedName(ContextVec);
        writeParameters(FunctionDecl);
        writeQualifiers(FunctionDecl);
    }

    writeBody(FunctionDecl);
    writeEnding();
}

void FunctionGenerator::dump(llvm::raw_ostream &OStream) const
{
    for (const auto &Include : Includes_)
        OStream << Include << "\n";

    OStream << "\n" << StrStream_.str() << "\n";

    /* Close namespaces which are still open */
    auto Size = ActiveNamespaces_.size();
    if (Size) {
        while (Size--)
            OStream << "}\n";

        if (!Configuration_->trimOutput())
            OStream << '\n';
    }
}

void FunctionGenerator::clear()
{
    ActiveNamespaces_.clear();
    Includes_.clear();
    StrStream_.clear();
}

void FunctionGenerator::writeNamespaceDefinitions(
    const llvm::SmallVector<const clang::DeclContext *, 8> &ContextVec)
{
    if (!Configuration_->namespaceDefinitions())
        return;

    std::vector<const clang::NamespaceDecl *>::size_type OkIndex = 0;

    for (auto Context : ContextVec) {
        auto NamespaceDecl = clang::dyn_cast<clang::NamespaceDecl>(Context);
        if (!NamespaceDecl)
            continue;

        NamespaceDecl = NamespaceDecl->getCanonicalDecl();

        auto Size = ActiveNamespaces_.size();
        if (OkIndex < Size) {
            if (NamespaceDecl == ActiveNamespaces_[OkIndex]) {
                /* Namespace is already open. */
                ++OkIndex;
                continue;
            }

            /*
             * Example:
             *
             *      namespace a {
             *      namespace b {
             *      ...
             *      }
             *      namespace c {       <---
             *      ...
             *      }
             *
             * Namespaces "a" and "b" are currently active. To open the new
             * namespace "c" the currently opened namespace "b" must be closed.
             * After that "c" can be opened.
             */

            ActiveNamespaces_.resize(OkIndex);

            while (Size-- > OkIndex)
                StrStream_ << "}\n";

            if (!Configuration_->trimOutput())
                StrStream_ << '\n';
        }

        /*
         * Example:
         *      namespace a {
         *      namespace c {       <---
         *      ...
         *      }
         *      }
         *
         * Namespace "a" is already opened and namespace "c"
         * comes into play. Open the new namespace.
         */

        ActiveNamespaces_.push_back(NamespaceDecl);
        ++OkIndex;

        StrStream_ << "namespace " << *NamespaceDecl << " {\n";

        if (!Configuration_->trimOutput())
            StrStream_ << '\n';
    }

    /* Close namespaces which are no longer present */
    auto Size = ActiveNamespaces_.size();
    if (OkIndex < Size) {
        ActiveNamespaces_.resize(OkIndex);

        while (Size-- > OkIndex)
            StrStream_ << "}\n";

        if (!Configuration_->trimOutput())
            StrStream_ << '\n';
    }
}

void FunctionGenerator::writeTemplateParameters(
    const clang::FunctionDecl *FunctionDecl)
{
    llvm::SmallVector<const clang::DeclContext *, 8> DeclContextVec;

    util::decl::getFullContext(FunctionDecl, DeclContextVec);

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

void FunctionGenerator::writeTemplateParameters(
    const clang::TemplateParameterList *List)
{
    StrStream_ << "template <";

    auto Begin = List->begin();
    auto End = List->end();

    for (auto It = Begin; It != End; ++It) {
        auto TTPDecl = clang::dyn_cast<clang::TemplateTypeParmDecl>(*It);
        auto NonTTPDecl = clang::dyn_cast<clang::NonTypeTemplateParmDecl>(*It);

        if (It != Begin)
            StrStream_ << ", ";

        if (TTPDecl) {
            StrStream_ << "typename ";

            if (TTPDecl->isParameterPack())
                StrStream_ << "... ";

        } else if (NonTTPDecl) {
            auto Policy = NonTTPDecl->getASTContext().getPrintingPolicy();
            StrStream_ << NonTTPDecl->getType().getAsString(Policy) << " ";
        }

        StrStream_ << (*It)->getName();
    }

    StrStream_ << "> ";
}

void FunctionGenerator::writeReturnType(const clang::FunctionDecl *FunctionDecl)
{
    if (!util::decl::hasReturnType(FunctionDecl))
        return;

    auto QualType = FunctionDecl->getReturnType();
    auto &PrintingPolicy = FunctionDecl->getASTContext().getPrintingPolicy();

    QualType.print(StrStream_, PrintingPolicy);

    /*
     * Output for reference and pointer types:
     *      "type &"
     *      "type *"
     * Output for other types:
     *      "type "
     */
    if (!QualType->isReferenceType() && !QualType->isPointerType())
        StrStream_ << " ";
}

void FunctionGenerator::writeTrailingFunctionStart()
{
    StrStream_ << "auto ";
}

void FunctionGenerator::writeTrailingReturnType(const clang::FunctionDecl *FunctionDecl)
{
    if (!util::decl::hasReturnType(FunctionDecl))
        return;

    StrStream_ << "-> ";

    auto QualType = FunctionDecl->getReturnType();
    auto &PrintingPolicy = FunctionDecl->getASTContext().getPrintingPolicy();

    QualType.print(StrStream_, PrintingPolicy);

    StrStream_ << " ";
}

void FunctionGenerator::writeFullQualifiedName(
    const llvm::SmallVector<const clang::DeclContext *, 8> &ContextVec)
{
    if (ContextVec.empty())
        return;

    auto &ASTContext = ContextVec[0]->getParentASTContext();
    auto PrintingPolicy = ASTContext.getPrintingPolicy();
    PrintingPolicy.SuppressScope = true;

    for (const auto &Context : ContextVec) {
        auto NamespaceDecl = clang::dyn_cast<clang::NamespaceDecl>(Context);
        if (NamespaceDecl) {
            if (!Configuration_->namespaceDefinitions())
                StrStream_ << *NamespaceDecl << "::";

            continue;
        }

        auto RecordDecl = clang::dyn_cast<clang::RecordDecl>(Context);
        if (RecordDecl) {
            auto Type = clang::QualType(RecordDecl->getTypeForDecl(), 0);
            Type.print(StrStream_, PrintingPolicy);
            StrStream_ << "::";
            continue;
        }

        auto CtorDecl = clang::dyn_cast<clang::CXXConstructorDecl>(Context);
        if (CtorDecl) {
            StrStream_ << *CtorDecl->getParent();
            continue;
        }

        auto DtorDecl = clang::dyn_cast<clang::CXXDestructorDecl>(Context);
        if (DtorDecl) {
            StrStream_ << '~' << *DtorDecl->getParent();
            continue;
        }

        auto FDecl = clang::dyn_cast<clang::FunctionDecl>(Context);
        if (FDecl) {
            StrStream_ << *FDecl;
            continue;
        }
    }
}

void FunctionGenerator::writeParameters(const clang::FunctionDecl *FunctionDecl)
{
    auto &PrintingPolicy = FunctionDecl->getASTContext().getPrintingPolicy();

    StrStream_ << "(";

    auto Parameters = FunctionDecl->parameters();
    auto Size = Parameters.size();

    for (size_t i = 0; i < Size; ++i) {
        llvm::SmallString<64> Buffer;
        llvm::raw_svector_ostream BufferStream(Buffer);

        auto Name = Parameters[i]->getName();
        auto QualType = Parameters[i]->getType();

        if (i > 0)
            StrStream_ << ", ";

        /* 
         * QualTypes can look like this when written to a stream:
         *      1)  "type"
         *      2)  "type *" / "type &"
         *      3)  type (*)(...)
         *
         * Here is the rough overall approach for writing a parameter 
         * to the stream:
         *
         *      1) Split the type strings (see above) in two parts at the 
         *          last '*' or '&' character. If there is no such character
         *          there is only one part which covers the whole type string.
         *      2) Write the first part of the type the stream.
         *      3) Write the parameter name to the stream.
         *      4) Write the second part of the type to the stream.
         */

        QualType.print(BufferStream, PrintingPolicy);

        /* Check if we have a pointer or reference type. */
        auto Index = Buffer.find_last_of("*&");
        if (Index != llvm::StringRef::npos)
            ++Index;

        /* Handle the first (and maybe only) part of the type string. */
        StrStream_ << Buffer.substr(0, Index);

        /* Append a space to non-pointer and non reference types */
        if (Index == llvm::StringRef::npos)
            StrStream_ << ' ';

        /* Handle with the parameter name. */
        if (Name.empty())
            StrStream_ << "arg" << i + 1;
        else
            StrStream_ << Name;

        /* If necessary, handle the second part of the type string. */
        if (Index < Buffer.size())
            StrStream_ << Buffer.substr(Index);
    }

    if (FunctionDecl->isVariadic()) {
        if (Size)
            StrStream_ << ", ";

        StrStream_ << "...";
    }

    StrStream_ << ")";
}

void FunctionGenerator::writeQualifiers(const clang::FunctionDecl *FunctionDecl)
{
    auto MethodDecl = clang::dyn_cast<clang::CXXMethodDecl>(FunctionDecl);
    if (MethodDecl) {
        if (MethodDecl->isConst())
            StrStream_ << " const";

        switch (MethodDecl->getRefQualifier()) {
        case clang::RefQualifierKind::RQ_LValue:
            StrStream_ << " &";
            break;
        case clang::RefQualifierKind::RQ_RValue:
            StrStream_ << " &&";
            break;
        case clang::RefQualifierKind::RQ_None:
        default:
            break;
        }
    }

    StrStream_ << ' ';
}

void FunctionGenerator::writeBody(const clang::FunctionDecl *FunctionDecl)
{
    if (Configuration_->implementAccessors()) {
        if (tryWriteGetAccessor(FunctionDecl))
            return;

        if (tryWriteSetAccessor(FunctionDecl))
            return;
    }

    if (Configuration_->implementConversions()) {
        if (tryWriteConversionStatement(FunctionDecl))
            return;
    }

    if (Configuration_->implementStubs()) {
        if (tryWriteReturnStatement(FunctionDecl))
            return;
    }

    StrStream_ << "{}";
}

void FunctionGenerator::writeEnding()
{
    /*
     * The output of 'fgen' is supposed to be piped to 'clang-format'.
     * Sadly, 'clang-format' does not automatically insert an empty line
     * between two functions. So we add an extra empty line here, except
     * if asked to not do so.
     */
    if (Configuration_->trimOutput())
        StrStream_ << "\n";
    else
        StrStream_ << "\n\n";
}

bool FunctionGenerator::tryWriteConversionStatement(
    const clang::FunctionDecl *FunctionDecl)
{
    auto ConvDecl = clang::dyn_cast<clang::CXXConversionDecl>(FunctionDecl);
    if (!ConvDecl)
        return false;

    auto RecordDecl = ConvDecl->getParent();
    auto ReturnType = ConvDecl->getReturnType();

    auto TypePred = [ReturnType](clang::QualType FieldType) {
        return util::type::returnAssignmentOk(ReturnType, FieldType);
    };

    auto TypeDecl = bestFieldDeclMatch(RecordDecl, TypePred);
    if (!TypeDecl)
        return false;

    StrStream_ << "{ return " << TypeDecl->getName() << "; }";

    return true;
}

bool FunctionGenerator::tryWriteReturnStatement(
    const clang::FunctionDecl *FunctionDecl)
{
    auto ReturnType = FunctionDecl->getReturnType();

    if (ReturnType->isVoidType())
        return false;

    if (ReturnType->isBooleanType()) {
        StrStream_ << "{ return false; }";
        return true;
    }

    if (ReturnType->isIntegerType() || ReturnType->isPointerType()) {
        StrStream_ << "{ return 0; }";
        return true;
    }

    if (ReturnType->isFloatingType()) {
        StrStream_ << "{ return 0.0; }";
        return true;
    }

    if (util::type::hasDefaultConstructor(ReturnType)) {
        auto &Policy = FunctionDecl->getASTContext().getPrintingPolicy();

        StrStream_ << "{ return ";
        ReturnType.print(StrStream_, Policy);
        StrStream_ << "(); }";
        return true;
    }

    if (ReturnType->isReferenceType()) {
        auto MethodDecl = clang::dyn_cast<clang::CXXMethodDecl>(FunctionDecl);

        if (MethodDecl && !MethodDecl->isStatic()) {
            auto ThisType = MethodDecl->getThisType();
            auto RecordType = ThisType->getPointeeType();

            if (util::type::returnAssignmentOk(ReturnType, RecordType)) {
                StrStream_ << "{ return *this; }";
                return true;
            }
        }

        auto NonRefType = ReturnType.getNonReferenceType();

        bool IsBuiltIn = NonRefType->isBuiltinType();
        if (IsBuiltIn || util::type::hasDefaultConstructor(NonRefType)) {
            auto &Policy = FunctionDecl->getASTContext().getPrintingPolicy();
            /*
             * Declare a static variable and return it, if the type
             * is default constructible.
             */
            StrStream_ << "{ static ";
            NonRefType.print(StrStream_, Policy);
            StrStream_ << " stub_dummy_; return stub_dummy_; }";

            return true;
        }
    }

    return false;
}

bool FunctionGenerator::tryWriteGetAccessor(
    const clang::FunctionDecl *FunctionDecl)
{
    if (FunctionDecl->getReturnType()->isVoidType())
        return false;

    auto MethodDecl = clang::dyn_cast<clang::CXXMethodDecl>(FunctionDecl);
    if (MethodDecl)
        return tryWriteCXXGetAccessor(MethodDecl);

    return tryWriteCGetAccessor(FunctionDecl);
}

bool FunctionGenerator::tryWriteCGetAccessor(
    const clang::FunctionDecl *FunctionDecl)
{
    /*
     * We define a get accessor function in C looks like this:
     *      a function_name(const struct b *p);
     *
     * Check if the current "FunctionDecl" matches with this
     * definition.
     */

    auto Parameters = FunctionDecl->parameters();

    if (Parameters.size() != 1)
        return false;

    auto Type = Parameters[0]->getType()->getPointeeType();
    if (Type.isNull() || !Type.isConstQualified())
        return false;

    auto RecordType = Type->getAs<clang::RecordType>();
    if (!RecordType)
        return false;

    auto RecordDecl = RecordType->getDecl();
    auto Name = FunctionDecl->getName();

    const auto ReturnType = FunctionDecl->getReturnType();

    auto TypePred = [ReturnType](clang::QualType FieldType) {
        return util::type::returnAssignmentOk(ReturnType, FieldType);
    };

    auto FieldDecl = bestFieldDeclMatch(RecordDecl, Name, TypePred);
    if (!FieldDecl)
        return false;

    StrStream_ << "{ return "
               << util::decl::getNameOrDefault(Parameters[0], "arg1") << "->"
               << FieldDecl->getName() << "; }";

    return true;
}

bool FunctionGenerator::tryWriteCXXGetAccessor(
    const clang::CXXMethodDecl *MethodDecl)
{
    /*
     * We define a get accessor function in C++ looks like this:
     *      class a {
     *          ...
     *          b function_name() const;
     *          ...
     *      };
     *
     * Check if the current "MethodDecl" matches with this
     * definition.
     */

    switch (MethodDecl->getKind()) {
    case clang::Decl::CXXConstructor:
    case clang::Decl::CXXDestructor:
    case clang::Decl::CXXConversion:
        return false;
    default:
        break;
    }

    if (!MethodDecl->parameters().empty())
        return false;

//    if (!MethodDecl->isConst()) {
//        /*
//         * Check if there is an const overload of this function.
//         * The following pattern is quite common in C++:
//         *
//         *      class a {
//         *      ...
//         *          b &get();
//         *          const b& get() const;
//         *      ...
//         *      };
//         *
//         * If we detect the above pattern, we will fill out the
//         * non-const version, too.
//         */
//        auto DeclName = MethodDecl->getDeclName();
//        auto Candidates = MethodDecl->getLookupParent()->lookup(DeclName);
//
//        auto Pred = [](const clang::NamedDecl *Decl) {
//            auto MethodDecl = clang::dyn_cast<clang::CXXMethodDecl>(Decl);
//            if (!MethodDecl)
//                return false;
//
//            return MethodDecl->isConst() && MethodDecl->parameters().empty();
//        };
//
//        if (llvm::none_of(Candidates, Pred))
//            return false;
//    }

    auto RecordDecl = MethodDecl->getParent();
    auto Name = MethodDecl->getName();
    const auto ReturnType = MethodDecl->getReturnType();

    auto TypePred = [ReturnType](clang::QualType FieldType) {
        return util::type::returnAssignmentOk(ReturnType, FieldType);
    };

    auto FieldDecl = bestFieldDeclMatch(RecordDecl, Name, TypePred);
    if (!FieldDecl)
        return false;

    auto RefQualifier = MethodDecl->getRefQualifier();
    auto IsRValue = (RefQualifier == clang::RefQualifierKind::RQ_RValue);

    bool UseMove = IsRValue && useMoveAssignment(FieldDecl->getType());

    StrStream_ << "{ return ";

    if (UseMove)
        StrStream_ << "std::move(";

    StrStream_ << FieldDecl->getName();

    if (UseMove)
        StrStream_ << ")";

    StrStream_ << "; }";

    return true;
}

bool FunctionGenerator::tryWriteCSetAccessor(
    const clang::FunctionDecl *FunctionDecl)
{
    /*
     * We define a set accessor function in C looks like this:
     *      void function_name(struct a *p, b val);
     *
     * Check if the current "FunctionDecl" matches with this
     * definition.
     */

    auto Parameters = FunctionDecl->parameters();

    if (Parameters.size() != 2)
        return false;

    auto Parameter1Type = Parameters[0]->getType()->getPointeeType();
    if (Parameter1Type.isNull() || Parameter1Type.isConstQualified())
        return false;

    auto RecordType = Parameter1Type->getAs<clang::RecordType>();
    if (!RecordType)
        return false;

    auto RecordDecl = RecordType->getDecl();
    auto Name = FunctionDecl->getName();

    const auto RHSType = Parameters[1]->getType();

    auto TypePred = [RHSType](clang::QualType FieldType) {
        return util::type::variableAssignmentOk(FieldType, RHSType);
    };

    auto FieldDecl = bestFieldDeclMatch(RecordDecl, Name, TypePred);
    if (!FieldDecl)
        return false;

    StrStream_ << "{ " << util::decl::getNameOrDefault(Parameters[0], "arg1")
               << "->" << FieldDecl->getName() << " = "
               << util::decl::getNameOrDefault(Parameters[1], "arg2") << "; }";

    return true;
}

bool FunctionGenerator::tryWriteCXXSetAccessor(
    const clang::CXXMethodDecl *MethodDecl)
{
    /*
     * We define a set accessor function in C++ looks like this:
     *      class a {
     *          ...
     *          void function_name(b val);
     *          ...
     *      };
     *
     * Check if the current "MethodDecl" matches with this
     * definition.
     */

    switch (MethodDecl->getKind()) {
    case clang::Decl::CXXConstructor:
    case clang::Decl::CXXDestructor:
    case clang::Decl::CXXConversion:
        return false;
    default:
        break;
    }

    if (MethodDecl->isConst())
        return false;

    auto Parameters = MethodDecl->parameters();

    if (Parameters.size() != 1)
        return false;

    auto RecordDecl = MethodDecl->getParent();
    auto Name = MethodDecl->getName();

    auto RHSType = Parameters[0]->getType();

    /* Treat rvalue references as value types */
    if (RHSType->isRValueReferenceType())
        RHSType = RHSType.getNonReferenceType();

    auto TypePred = [RHSType](clang::QualType LHSType) {
        return util::type::variableAssignmentOk(LHSType, RHSType);
    };

    auto FieldDecl = bestFieldDeclMatch(RecordDecl, Name, TypePred);
    if (!FieldDecl)
        return false;

    StrStream_ << "{ " << FieldDecl->getName() << " = ";

    /* Try to use the move assignment operator if available */
    bool UseMove = useMoveAssignment(RHSType);

    if (UseMove)
        StrStream_ << "std::move(";

    StrStream_ << util::decl::getNameOrDefault(Parameters[0], "arg1");

    if (UseMove)
        StrStream_ << ")";

    StrStream_ << "; }";

    return true;
}

bool FunctionGenerator::tryWriteSetAccessor(
    const clang::FunctionDecl *FunctionDecl)
{
    if (!FunctionDecl->getReturnType()->isVoidType())
        return false;

    auto MethodDecl = clang::dyn_cast<clang::CXXMethodDecl>(FunctionDecl);
    if (MethodDecl)
        return tryWriteCXXSetAccessor(MethodDecl);

    return tryWriteCSetAccessor(FunctionDecl);
}

bool FunctionGenerator::useMoveAssignment(clang::QualType Type)
{
    if (!Configuration_->allowMove())
        return false;

    if (Type->isPointerType() || Type->isReferenceType())
        return false;

    if (!Type->isTemplateTypeParmType() && !util::type::hasMoveAssignment(Type))
        return false;

    return addInclude("#include <utility>");
}

bool FunctionGenerator::addInclude(std::string Include)
{
    const auto &[It, Ok] = Includes_.insert(std::move(Include));

    return Ok || It != Includes_.end();
}
