// Copyright (c) 2017-2018 Florian Wende (flwende@gmail.com)
//
// Distributed under the BSD 2-clause Software License
// (See accompanying file LICENSE)

#if !defined(TRAFO_DATA_LAYOUT_CLASS_META_DATA_HPP)
#define TRAFO_DATA_LAYOUT_CLASS_META_DATA_HPP

#include <string>
#include <vector>
#include <cstdint>
#include <misc/matcher.hpp>

#if !defined(TRAFO_NAMESPACE)
    #define TRAFO_NAMESPACE fw
#endif

namespace TRAFO_NAMESPACE
{
    namespace internal
    {
#if defined(old)
        class ClassMetaData
        {
            struct AccessSpecifier
            {
                const clang::AccessSpecDecl& decl;
                const clang::SourceLocation sourceLocation;
                const clang::SourceRange sourceRange;
                const clang::SourceLocation scopeBegin;

                AccessSpecifier(const clang::AccessSpecDecl& decl)
                    :
                    decl(decl),
                    sourceLocation(decl.getAccessSpecifierLoc()),
                    sourceRange(decl.getSourceRange()),
                    scopeBegin(decl.getColonLoc().getLocWithOffset(1))
                { ; }
            };

            struct Constructor
            {
                const clang::CXXConstructorDecl& decl;

                Constructor(const clang::CXXConstructorDecl& decl)
                    :
                    decl(decl)
                { ; }
            };

            struct MemberField
            {
                const clang::FieldDecl& decl;
                const std::string name;
                const std::string typeName;
                const bool isPublic;
                const bool isConst;
                const bool isFundamentalOrTemplateType;
                
                MemberField(const clang::FieldDecl& decl, const std::string typeName, const std::string name, const bool isPublic, const bool isConst, const bool isFundamentalOrTemplateType)
                    :
                    decl(decl),
                    name(name),
                    typeName(typeName),
                    isPublic(isPublic),
                    isConst(isConst),
                    isFundamentalOrTemplateType(isFundamentalOrTemplateType)
                { ; }
            };
            
            void collectMetaData()
            {
                Matcher matcher;

                // match all (public, private) fields of the class and collect information: 
                // declaration, const qualifier, fundamental type, type name, name
                std::string publicFieldTypeName;
                for (auto field : decl.fields())
                {
                    const clang::QualType qualType = field->getType();
                    const clang::Type* type = qualType.getTypePtrOrNull();

                    const std::string name = field->getType().getAsString();
                    const std::string typeName = field->getNameAsString();
                    const bool isPublic = Matcher::testDecl(*field, clang::ast_matchers::fieldDecl(clang::ast_matchers::isPublic()), context);
                    const bool isConstant = qualType.getQualifiers().hasConst();
                    const bool isFundamentalOrTemplateType = (type != nullptr ? (type->isFundamentalType() || type->isTemplateTypeParmType()) : false);
                    fields.emplace_back(*field, name, typeName, isPublic, isConstant, isFundamentalOrTemplateType);

                    if (isPublic)
                    {
                        const MemberField& thisField = fields.back();

                        if (numPublicFields == 0)
                        {
                            publicFieldTypeName = thisField.typeName;
                        }
                        else
                        {
                            hasMultiplePublicFieldTypes |= (thisField.typeName != publicFieldTypeName);
                        }

                        hasNonFundamentalPublicFields |= !(thisField.isFundamentalOrTemplateType);

                        ++numPublicFields;
                    }
                }
                
                // proxy class candidates must have at least 1 public field
                isProxyClassCandidate &= (numPublicFields > 0);
                if (!isProxyClassCandidate) return;

                // proxy class candidates must have fundamental public fields
                isProxyClassCandidate &= !hasNonFundamentalPublicFields;

                if (isProxyClassCandidate);
                {
                    using namespace clang::ast_matchers;

                    // match public and private access specifier
                    matcher.addMatcher(accessSpecDecl(hasParent(cxxRecordDecl(allOf(hasName(name), unless(isTemplateInstantiation()))))).bind("accessSpecDecl"), \
                        [&] (const MatchFinder::MatchResult& result) mutable \
                        {
                            if (const clang::AccessSpecDecl* decl = result.Nodes.getNodeAs<clang::AccessSpecDecl>("accessSpecDecl"))
                            {
                                publicAccess.emplace_back(*decl);
                            }
                        });
                    
                    // match all (public, private) constructors
                #define MATCH(MODIFIER, VARIABLE) \
                    matcher.addMatcher(cxxConstructorDecl(allOf(MODIFIER(), hasParent(cxxRecordDecl(allOf(hasName(name), unless(isTemplateInstantiation())))))).bind("constructorDecl"), \
                        [&] (const MatchFinder::MatchResult& result) mutable \
                        { \
                            if (const clang::CXXConstructorDecl* decl = result.Nodes.getNodeAs<clang::CXXConstructorDecl>("constructorDecl")) \
                            { \
                                VARIABLE.emplace_back(*decl); \
                            } \
                        })
                        
                    MATCH(isPublic, publicConstructors);
                    MATCH(isPrivate, privateConstructors);
                #undef MATCH

                    matcher.run(context);
                    matcher.clear();
                }
            }

        public:

            const clang::CXXRecordDecl& decl;
            clang::ASTContext& context;
            const std::string name;
            const bool isStruct;
            const std::uint32_t numFields;
            std::uint32_t numPublicFields;
            bool hasNonFundamentalPublicFields;
            bool hasMultiplePublicFieldTypes;
            bool isProxyClassCandidate;
            std::vector<AccessSpecifier> publicAccess;
            std::vector<AccessSpecifier> privateAccess;
            std::vector<Constructor> publicConstructors;
            std::vector<Constructor> privateConstructors;
            std::vector<MemberField> fields;

            // proxy class candidates should not have any of these properties: abstract, polymorphic, empty AND
            // proxy class candidates must be class definitions and no specializations in case of template classes
            // note: a template class specialization is of type CXXRecordDecl, and we need to check for its describing template class
            //       (if it does not exist, then it is a normal C++ class)
            ClassMetaData(const clang::CXXRecordDecl& decl, clang::ASTContext& context)
                :
                decl(decl),
                context(context),
                name(decl.getNameAsString()),                
                isStruct(Matcher::testDecl(decl, clang::ast_matchers::recordDecl(clang::ast_matchers::isStruct()), context)),
                numFields(std::distance(decl.field_begin(), decl.field_end())),
                numPublicFields(0),
                hasNonFundamentalPublicFields(false),
                hasMultiplePublicFieldTypes(false),
                isProxyClassCandidate(numFields > 0 && (decl.getDescribedClassTemplate() == nullptr) &&
                                        !(decl.isAbstract() || decl.isPolymorphic() || decl.isEmpty()))
            {
                if (isProxyClassCandidate)
                {
                    collectMetaData();
                }
            }

            clang::SourceLocation publicScopeStart()
            {
                if (publicAccess.size() > 0)
                {
                    return publicAccess[0].scopeBegin;
                }
                else if (isStruct)
                {
                    return decl.getBraceRange().getBegin().getLocWithOffset(1);
                }
                else
                {
                    return decl.getBraceRange().getEnd();
                }
            }
        };
#else
        class ClassMetaData
        {
        public:

            const std::string name;
            const bool isTemplated;

            ClassMetaData(const std::string name, const bool isTemplated)
                :
                name(name),
                isTemplated(isTemplated)
            { ; }

            virtual void printInfo(clang::SourceManager& sm, const std::string indent = "") const = 0;
        };

        class CXXClassMetaData : public ClassMetaData
        {
            using Base = ClassMetaData;

        public:

            using Base::name;
            const clang::CXXRecordDecl& decl;
            clang::SourceRange sourceRange;

            CXXClassMetaData(const std::string name, const clang::CXXRecordDecl& decl)
                :
                Base(name, false),
                decl(decl),
                sourceRange(decl.getDescribedClassTemplate() ? decl.getDescribedClassTemplate()->getSourceRange() : decl.getSourceRange())
            { ; }

            virtual void printInfo(clang::SourceManager& sm, const std::string indent = "") const
            {
                std::cout << indent << "C++ Class: " << name << std::endl;
                std::cout << indent << "\tdefinition: " << sourceRange.printToString(sm) << std::endl;
            }
        };

        class TemplateClassMetaData : public ClassMetaData
        {
            using Base = ClassMetaData;

            class Declaration
            {
            public:

                const clang::ClassTemplateDecl& decl;
                clang::SourceRange sourceRange;
                const bool isDefinition;

                Declaration(const clang::ClassTemplateDecl& decl)
                    :
                    decl(decl),
                    sourceRange(decl.getSourceRange()),
                    isDefinition(decl.isThisDeclarationADefinition())
                { ; }
            };

            class Definition : public CXXClassMetaData
            {
                using Base = CXXClassMetaData;

            public:

                using Base::name;
                using Base::decl;
                using Base::sourceRange;
                bool isTemplateSpecialization;

                Definition(const std::string name, const clang::CXXRecordDecl& decl, bool isTemplateSpecialization)
                    :
                    Base(name, decl),
                    isTemplateSpecialization(isTemplateSpecialization)
                { ; }

                void printInfo(clang::SourceManager& sm, const std::string indent = "") const
                {
                    Base::printInfo(sm, indent);
                    std::cout << indent << "\tthis is a template class " << (isTemplateSpecialization ? "specialization" : "definition") << std::endl;
                }
            };

        public:
            
            using Base::name;
            Declaration declaration;
            std::vector<Definition> definitions;

            TemplateClassMetaData(const std::string name, const clang::ClassTemplateDecl& decl)
                :
                Base(name, true),
                declaration(decl)
            { 
                if (declaration.isDefinition)
                {
                    definitions.emplace_back(name, *(decl.getTemplatedDecl()), false);
                }
            }

            bool addDefinition(const clang::CXXRecordDecl& decl, const bool isTemplateSpecialization = false)
            {
                if (!decl.isThisDeclarationADefinition()) return false;

                clang::ClassTemplateDecl* tDecl = decl.getDescribedClassTemplate();
                clang::SourceRange sourceRange = (tDecl != nullptr ? tDecl->getSourceRange() : decl.getSourceRange());

                const std::string className = decl.getNameAsString();
                if (name == className)
                {
                    for (std::size_t i = 0; i < definitions.size(); ++i)
                    {
                        if (sourceRange == definitions[i].sourceRange) 
                        {
                            if (!isTemplateSpecialization)
                            {
                                definitions[i].isTemplateSpecialization = false;
                            }

                            return true;
                        }
                    }
                    
                    // no? then add this definition!
                    definitions.emplace_back(className, decl, isTemplateSpecialization);

                    return true;
                }

                return false;
            }

            virtual void printInfo(clang::SourceManager& sm, const std::string indent = "") const
            {
                std::cout << "C++ Template Class: " << name << std::endl;
                std::cout << "\tdeclaration is definition: " << (declaration.isDefinition ? "yes" : "no") << std::endl;
                std::cout << "\tdeclaration: " << declaration.sourceRange.printToString(sm) << std::endl;
                std::cout << "\tdefinitions..." << std::endl;
                for (auto definition : definitions)
                {
                    definition.printInfo(sm, "\t\t");
                }
                std::cout << "\t...definitions" << std::endl;
            }
        };
#endif
    }
}

#endif