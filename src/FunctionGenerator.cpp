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

template<typename Pred>
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

    Name = extractRelevantSection(Name);
    auto MaxEditDistance = Name.size();
    auto NameSize = static_cast<int>(Name.size());

    const clang::FieldDecl *BestMatch = nullptr;
    unsigned int BestEditDistance = MaxEditDistance;

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
         * for names that cannot possibly be have a smaller edit distance
         * with this simple comparison.
         */
        if (std::abs(NameSize - FieldNameSize) > BestEditDistance)
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

FunctionGenerator::FunctionGenerator(std::string &Buffer,
                                     std::unordered_set<std::string> &Includes,
                                     const FGenConfiguration &Configuration)
    : OStream_(Buffer), Includes_(&Includes), Configuration_(&Configuration)
{
    OStream_.SetUnbuffered();
}

void FunctionGenerator::write(const clang::FunctionDecl *FunctionDecl)
{
    writeTemplateParameters(FunctionDecl);
    writeReturnType(FunctionDecl);
    writeFullName(FunctionDecl);
    writeParameters(FunctionDecl);
    writeQualifiers(FunctionDecl);
    writeBody(FunctionDecl);
    writeEnding();
}

void FunctionGenerator::writeTemplateParameters(
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

void FunctionGenerator::writeTemplateParameters(
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

    OStream_ << "> ";
}

void FunctionGenerator::writeReturnType(const clang::FunctionDecl *FunctionDecl)
{
    /*
     * For some types of function definitions the return type
     * does not get written.
     */
    switch (FunctionDecl->getKind()) {
    case clang::Decl::CXXConstructor:
    case clang::Decl::CXXDestructor:
    case clang::Decl::CXXConversion:
        return;
    default:
        break;
    }

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

void FunctionGenerator::writeFullName(
    const clang::FunctionDecl *const FunctionDecl)
{
    auto Namespaces = Configuration_->writeNamespaces();

    util::decl::printFullQualifiedName(FunctionDecl, OStream_, Namespaces);
}

void FunctionGenerator::writeParameters(const clang::FunctionDecl *FunctionDecl)
{
    auto &PrintingPolicy = FunctionDecl->getASTContext().getPrintingPolicy();

    OStream_ << "(";

    auto Parameters = FunctionDecl->parameters();

    for (std::size_t i = 0; i < Parameters.size(); ++i) {
        auto QualType = Parameters[i]->getType();

        if (i > 0)
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

        auto Name = Parameters[i]->getName();
        if (Name.empty())
            OStream_ << "arg" << i + 1;
        else
            OStream_ << Name;
    }

    OStream_ << ")";
}

void FunctionGenerator::writeQualifiers(const clang::FunctionDecl *FunctionDecl)
{
    auto MethodDecl = clang::dyn_cast<clang::CXXMethodDecl>(FunctionDecl);
    if (MethodDecl) {
        if (MethodDecl->isConst())
            OStream_ << " const";

        switch (MethodDecl->getRefQualifier()) {
        case clang::RefQualifierKind::RQ_LValue:
            OStream_ << " &";
            break;
        case clang::RefQualifierKind::RQ_RValue:
            OStream_ << " &&";
            break;
        case clang::RefQualifierKind::RQ_None:
        default:
            break;
        }
    }

    OStream_ << ' ';
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

    OStream_ << "{}";
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
        OStream_ << "\n";
    else
        OStream_ << "\n\n";
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
        return util::type::isReturnAssignmentOk(ReturnType, FieldType);
    };

    auto TypeDecl = bestFieldDeclMatch(RecordDecl, TypePred);
    if (!TypeDecl)
        return false;

    OStream_ << "{ return " << TypeDecl->getName() << "; }";

    return true;
}

bool FunctionGenerator::tryWriteReturnStatement(
    const clang::FunctionDecl *FunctionDecl)
{
    auto ReturnType = FunctionDecl->getReturnType();

    if (ReturnType->isVoidType())
        return false;

    if (ReturnType->isBooleanType()) {
        OStream_ << "{ return false; }";
        return true;
    }

    if (ReturnType->isIntegerType() || ReturnType->isPointerType()) {
        OStream_ << "{ return 0; }";
        return true;
    }

    if (ReturnType->isFloatingType()) {
        OStream_ << "{ return 0.0; }";
        return true;
    }

    if (util::type::hasDefaultConstructor(ReturnType)) {
        auto &Policy = FunctionDecl->getASTContext().getPrintingPolicy();

        OStream_ << "{ return ";
        ReturnType.print(OStream_, Policy);
        OStream_ << "(); }";
        return true;
    }

    if (ReturnType->isReferenceType()) {
        auto NonRefType = ReturnType.getNonReferenceType();
        auto &Policy = FunctionDecl->getASTContext().getPrintingPolicy();

        bool IsBuiltIn = NonRefType->isBuiltinType();
        if (IsBuiltIn || util::type::hasDefaultConstructor(NonRefType)) {
            /*
             * Declare a static variable and return it, if the type
             * is default constructible.
             */
            OStream_ << "{ static ";
            NonRefType.print(OStream_, Policy);
            OStream_ << " stub_dummy_; return stub_dummy_; }";

            return true;
        }

        //         auto MethodDecl =
        //         clang::dyn_cast<clang::CXXMethodDecl>(FunctionDecl); if
        //         (MethodDecl) {
        //             /*
        //              * TODO: comments and extra class or functions for
        //              * extracting fields of records for assignment
        //              */
        //             auto RecordDecl = MethodDecl->getParent();
        //             auto Name = MethodDecl->getName();
        //
        //             auto TypePred = [ReturnType](clang::QualType FieldType) {
        //                 return util::type::isReturnAssignmentOk(ReturnType,
        //                 FieldType);
        //             };
        //
        //             auto FieldDecl = bestFieldDeclMatch(RecordDecl, Name,
        //             TypePred); if (FieldDecl) {
        //                 OStream_ << "{\n    return " << FieldDecl->getName()
        //                          << ";\n}\n\n";
        //                 return true;
        //             }
        //         }
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
        return util::type::isReturnAssignmentOk(ReturnType, FieldType);
    };

    auto FieldDecl = bestFieldDeclMatch(RecordDecl, Name, TypePred);
    if (!FieldDecl)
        return false;

    OStream_ << "{ return "
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

    if (!MethodDecl->parameters().empty())
        return false;

    if (!MethodDecl->isConst()) {
        /*
         * Check if there is an const overload of this function.
         * The following pattern is quite common in C++:
         *
         *      class a {
         *      ...
         *          b &get();
         *          const b& get() const;
         *      ...
         *      };
         *
         * If we detect this pattern, we fill out the
         * non-const version, too.
         */
        auto DeclName = MethodDecl->getDeclName();
        auto Candidates = MethodDecl->getLookupParent()->lookup(DeclName);

        auto Pred = [](const clang::NamedDecl *Decl) {
            auto MethodDecl = clang::dyn_cast<clang::CXXMethodDecl>(Decl);
            if (!MethodDecl)
                return false;

            return MethodDecl->isConst() && MethodDecl->parameters().empty();
        };

        if (std::none_of(Candidates.begin(), Candidates.end(), Pred))
            return false;
    }

    auto RecordDecl = MethodDecl->getParent();
    auto Name = MethodDecl->getName();
    const auto ReturnType = MethodDecl->getReturnType();

    auto TypePred = [ReturnType](clang::QualType FieldType) {
        return util::type::isReturnAssignmentOk(ReturnType, FieldType);
    };

    auto FieldDecl = bestFieldDeclMatch(RecordDecl, Name, TypePred);
    if (!FieldDecl)
        return false;

    auto RefQualifier = MethodDecl->getRefQualifier();
    auto IsRValue = (RefQualifier == clang::RefQualifierKind::RQ_RValue);

    bool UseMove = IsRValue && useMoveAssignment(FieldDecl->getType());

    OStream_ << "{ return ";

    if (UseMove)
        OStream_ << "std::move(";

    OStream_ << FieldDecl->getName();

    if (UseMove)
        OStream_ << ")";

    OStream_ << "; }";

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
        return util::type::isVariableAssignmentOk(FieldType, RHSType);
    };

    auto FieldDecl = bestFieldDeclMatch(RecordDecl, Name, TypePred);
    if (!FieldDecl)
        return false;

    OStream_ << "{ " << util::decl::getNameOrDefault(Parameters[0], "arg1")
             << "->" << FieldDecl->getName() << " = "
             << util::decl::getNameOrDefault(Parameters[1], "arg2")
             << "; }";

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

    if (MethodDecl->isConst())
        return false;

    auto Parameters = MethodDecl->parameters();

    if (Parameters.size() != 1)
        return false;

    auto RecordDecl = MethodDecl->getParent();
    auto Name = MethodDecl->getName();

    const auto RHSType = Parameters[0]->getType();
    auto TypePred = [RHSType](clang::QualType FieldType) {
        return util::type::isVariableAssignmentOk(FieldType, RHSType);
    };

    auto FieldDecl = bestFieldDeclMatch(RecordDecl, Name, TypePred);
    if (!FieldDecl)
        return false;

    OStream_ << "{ " << FieldDecl->getName() << " = ";

    /* Try to use the move assignment operator if available */
    bool UseMove = useMoveAssignment(RHSType);

    if (UseMove)
        OStream_ << "std::move(";

    OStream_ << util::decl::getNameOrDefault(Parameters[0], "arg1");

    if (UseMove)
        OStream_ << ")";

    OStream_ << "; }";

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

    if (!util::type::hasMoveAssignment(Type))
        return false;

    return addInclude("#include <utility>");
}

bool FunctionGenerator::addInclude(std::string Include)
{
    std::unordered_set<std::string>::iterator It;
    bool Ok;

    std::tie(It, Ok) = Includes_->insert(std::move(Include));

    return Ok || It != Includes_->end();
}
