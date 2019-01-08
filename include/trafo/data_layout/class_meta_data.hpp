// Copyright (c) 2017-2018 Florian Wende (flwende@gmail.com)
//
// Distributed under the BSD 2-clause Software License
// (See accompanying file LICENSE)

#if !defined(TRAFO_DATA_LAYOUT_CLASS_META_DATA_HPP)
#define TRAFO_DATA_LAYOUT_CLASS_META_DATA_HPP

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <cstdint>
#include <misc/ast_helper.hpp>
#include <misc/matcher.hpp>
#include <misc/rewriter.hpp>
#include <misc/string_helper.hpp>

#if !defined(TRAFO_NAMESPACE)
    #define TRAFO_NAMESPACE fw
#endif

namespace TRAFO_NAMESPACE
{
    namespace internal
    {
        class ClassMetaData
        {
            class AccessSpecifier
            {
            public:

                enum class Type {Public = 0, Protected = 1, Private = 2};

                const clang::AccessSpecDecl& decl;
                const clang::SourceRange sourceRange;
                const clang::SourceLocation scopeBegin;
                const Type type;
                const std::string name;

                AccessSpecifier(const clang::AccessSpecDecl& decl, Type type)
                    :
                    decl(decl),
                    sourceRange(decl.getSourceRange()),
                    scopeBegin(decl.getColonLoc().getLocWithOffset(1)),
                    type(type),
                    name(type == Type::Public ? "public" : (type == Type::Protected ? "protected" : "private"))
                { ; }

                void printInfo(clang::SourceManager& sm, const std::string indent = "") const
                {
                    std::cout << indent << "* " << name << std::endl;
                    std::cout << indent << "\t+-> range: " << sourceRange.printToString(sm) << std::endl;
                    std::cout << indent << "\t+-> scope begin: " << scopeBegin.printToString(sm) << std::endl;
                }
            };

            class Field
            {
            public:
                            
                const clang::FieldDecl& decl;
                const clang::SourceRange sourceRange;
                const std::string name;
                std::unique_ptr<const clang::Type> type;
                const std::string typeName;
                const AccessSpecifier::Type access;
                const bool isPublic;
                const bool isProtected;
                const bool isPrivate;
                const bool isConst;
                const bool isFundamentalOrTemplated;
                
                Field(const clang::FieldDecl& decl, const AccessSpecifier::Type access)
                    :
                    decl(decl),
                    sourceRange(decl.getSourceRange()),
                    name(decl.getNameAsString()),
                    type(decl.getType().getTypePtrOrNull()),
                    typeName(decl.getType().getAsString()),
                    access(access),
                    isPublic(access == AccessSpecifier::Type::Public),
                    isProtected(access == AccessSpecifier::Type::Protected),
                    isPrivate(access == AccessSpecifier::Type::Private),
                    isConst(decl.getType().isConstQualified()),
                    isFundamentalOrTemplated(type != nullptr ? (type->isFundamentalType() || type->isTemplateTypeParmType()) : false)
                { ; }

                void printInfo(clang::SourceManager& sm, const std::string indent = "") const
                {
                    std::cout << indent << "* name=" << name << ", type=" << typeName << (isConst ? " (const qualified)" : "") << std::endl;
                    std::cout << indent << "\t+-> range: " << sourceRange.printToString(sm) << std::endl;
                    std::cout << indent << "\t+-> access: " << (isPublic ? "public" : (isProtected ? "protected" : "private")) << std::endl;
                    std::cout << indent << "\t+-> fundamental or templated: " << (isFundamentalOrTemplated ? "yes" : "no") << std::endl;
                }
            };

            struct Constructor
            {
                const clang::CXXConstructorDecl& decl;
                const clang::SourceRange sourceRange;
                const bool isDefaultConstructor;
                const bool isCopyConstructor;
                const bool hasBody;
                const AccessSpecifier::Type access;
                const bool isPublic;
                const bool isProtected;
                const bool isPrivate;

                Constructor(const clang::CXXConstructorDecl& decl, const AccessSpecifier::Type access)
                    :
                    decl(decl),
                    sourceRange(decl.getSourceRange()),
                    isDefaultConstructor(decl.isDefaultConstructor()),
                    isCopyConstructor(decl.isCopyConstructor()),
                    hasBody(decl.hasBody()),
                    access(access),
                    isPublic(access == AccessSpecifier::Type::Public),
                    isProtected(access == AccessSpecifier::Type::Protected),
                    isPrivate(access == AccessSpecifier::Type::Private)
                { ; }

                void printInfo(clang::SourceManager& sm, const std::string indent = "") const
                {
                    std::cout << indent << "* " << (isDefaultConstructor ? "default " : (isCopyConstructor ? "copy " : "")) << "constructor" << std::endl;
                    std::cout << indent << "\t+-> range: " << sourceRange.printToString(sm) << std::endl;
                    std::cout << indent << "\t+-> access: " << (isPublic ? "public" : (isProtected ? "protected" : "private")) << std::endl;
                    std::cout << indent << "\t+-> has body: " << (hasBody ? "yes" : "no") << std::endl;
                }
            };

            class Namespace
            {
            public:
            
                const clang::NamespaceDecl& decl;
                const clang::SourceRange sourceRange;
                const std::string name;

                Namespace(const clang::NamespaceDecl& decl)
                    :
                    decl(decl),
                    sourceRange(decl.getSourceRange()),
                    name(decl.getNameAsString())
                { ; }

                static std::vector<Namespace> getFromDecl(const clang::NamedDecl& decl)
                {
                    using namespace clang::ast_matchers;

                    std::vector<Namespace> namespaces;

                    Matcher matcher;
                    matcher.addMatcher(namespaceDecl(hasDescendant(namedDecl(hasName(decl.getNameAsString())))).bind("namespaceDecl"),
                        [&] (const MatchFinder::MatchResult& result) mutable
                        {
                            if (const clang::NamespaceDecl* decl = result.Nodes.getNodeAs<clang::NamespaceDecl>("namespaceDecl"))
                            {
                                namespaces.emplace_back(*decl);
                            }
                        });
                    matcher.run(decl.getASTContext());

                    return namespaces;
                }

                void printInfo(clang::SourceManager& sm, const std::string indent = "") const
                {
                    std::cout << indent << "* " << name << ", " << sourceRange.printToString(sm) << std::endl;
                }
            };

        public:

            class Declaration
            {
                std::unique_ptr<const clang::CXXRecordDecl> cxxRecordDecl;
                std::unique_ptr<const clang::ClassTemplateDecl> classTemplateDecl;

            public:
        
                const clang::SourceRange sourceRange;
                std::unique_ptr<const clang::TemplateParameterList> templateParameterList;
                const clang::SourceRange templateParameterListSourceRange;
                const std::string name;
                const std::vector<Namespace> namespaces;
                const bool isDefinition;
                const bool isStruct;
                const bool isClass;

                template <typename T>
                Declaration(const T& decl, const bool isDefinition)
                    :
                    cxxRecordDecl(ClassDecl::getTemplatedDecl(decl)), // is never nullptr
                    classTemplateDecl(ClassDecl::getDescribedClassTemplate(decl)), // might be nullptr
                    sourceRange(decl.getSourceRange()),
                    templateParameterList(classTemplateDecl ? classTemplateDecl->getTemplateParameters() : nullptr),
                    templateParameterListSourceRange(templateParameterList ? templateParameterList->getSourceRange() : clang::SourceRange()),
                    name(decl.getNameAsString()),
                    namespaces(Namespace::getFromDecl(decl)),
                    isDefinition(isDefinition),
                    isStruct(cxxRecordDecl->isStruct()),
                    isClass(cxxRecordDecl->isClass())
                { ; }

                void printInfo(clang::SourceManager& sm, const std::string indent = "") const
                {
                    std::cout << indent << "* DECLARATION:" << std::endl;
                    std::cout << indent << "\t+-> range: " << sourceRange.printToString(sm) << std::endl;
                    if (templateParameterList.get())
                    {
                        std::cout << indent << "\t+-> template parameter list range: " << templateParameterListSourceRange.printToString(sm) << std::endl;
                    }
                    std::cout << indent << "\t+-> namespace(s):" << std::endl;
                    for (auto& ns : namespaces)
                    {
                        ns.printInfo(sm, indent + "\t|\t");
                    }
                    std::cout << indent << "\t+-> is definition: " << (isDefinition ? "yes" : "no") << std::endl;
                }
            };

            class Definition
            {
                void testIfProxyClassIsCandidate()
                {
                    // not abstract, polymorphic, empty
                    isProxyClassCandidate &= !(decl.isAbstract() || decl.isPolymorphic() || decl.isEmpty());

                    // at least one public field
                    isProxyClassCandidate &= (ptrPublicFields.size() > 0);

                    // public fields should be all fundamental or templated and of the same type
                    bool hasMultiplePublicFieldTypes = false;
                    bool hasNonFundamentalPublicFields = false;
                    const std::string publicFieldTypeName = ptrPublicFields[0]->typeName;
                    for (auto& field : ptrPublicFields)
                    {
                        hasMultiplePublicFieldTypes |= (field->typeName != publicFieldTypeName);
                        hasNonFundamentalPublicFields |= !(field->isFundamentalOrTemplated);
                    }
                    isProxyClassCandidate &= !(hasMultiplePublicFieldTypes || hasNonFundamentalPublicFields);
                }

                std::unique_ptr<const clang::ClassTemplateDecl> classTemplateDecl;

            public:
            
                const Declaration& declaration;
                const clang::CXXRecordDecl& decl;
                const clang::SourceRange sourceRange;
                std::unique_ptr<const clang::TemplateParameterList> templateParameterList;
                const clang::SourceRange templateParameterListSourceRange;
                const std::string name;
                const bool isTemplatePartialSpecialization;
                std::vector<Field> fields;
                std::vector<std::unique_ptr<const Field>> ptrPublicFields;
                std::vector<std::unique_ptr<const Field>> ptrProtectedFields;
                std::vector<std::unique_ptr<const Field>> ptrPrivateFields;
                std::vector<AccessSpecifier> accessSpecifiers;
                std::unique_ptr<const AccessSpecifier> ptrPublicAccess;
                std::unique_ptr<const AccessSpecifier> ptrProtectedAccess;
                std::unique_ptr<const AccessSpecifier> ptrPrivateAccess;
                std::vector<Constructor> constructors;
                std::vector<std::unique_ptr<const Constructor>> ptrPublicConstructors;
                std::vector<std::unique_ptr<const Constructor>> ptrProtectedConstructors;
                std::vector<std::unique_ptr<const Constructor>> ptrPrivateConstructors;
                bool hasCopyConstructor;
                std::unique_ptr<const Constructor> copyConstructor;
                bool isProxyClassCandidate;

                Definition(const Declaration& declaration, const clang::CXXRecordDecl& decl, const bool isTemplatePartialSpecialization = false)
                    :
                    classTemplateDecl(ClassDecl::getDescribedClassTemplate(decl)), // can be nullptr
                    templateParameterList(classTemplateDecl ? classTemplateDecl->getTemplateParameters() : nullptr),
                    templateParameterListSourceRange(templateParameterList ? templateParameterList->getSourceRange() : clang::SourceRange()),
                    declaration(declaration),
                    decl(decl),
                    sourceRange(classTemplateDecl ? classTemplateDecl->getSourceRange() : decl.getSourceRange()),
                    name(decl.getNameAsString()),
                    isTemplatePartialSpecialization(isTemplatePartialSpecialization),
                    hasCopyConstructor(false),
                    isProxyClassCandidate(true)
                {
                    using namespace clang::ast_matchers;

                    Matcher matcher;
                    
                    // get fields
                    const std::size_t numFields = std::distance(decl.field_begin(), decl.field_end());
                    fields.reserve(numFields);
                #define ADD_MATCHER(ACCESS) \
                    matcher.addMatcher(fieldDecl(is ## ACCESS()).bind("fieldDecl"), \
                        [&] (const MatchFinder::MatchResult& result) mutable \
                        { \
                            if (const clang::FieldDecl* decl = result.Nodes.getNodeAs<clang::FieldDecl>("fieldDecl")) \
                            { \
                                fields.emplace_back(*decl, AccessSpecifier::Type::ACCESS); \
                                ptr ## ACCESS ## Fields.emplace_back(&fields.back()); \
                            } \
                        }, &decl)

                    ADD_MATCHER(Public);
                    ADD_MATCHER(Protected);
                    ADD_MATCHER(Private);
                #undef ADD_MATCHER

                    // get access specifier
                #define ADD_MATCHER(ACCESS) \
                    matcher.addMatcher(accessSpecDecl(is##ACCESS()).bind("accessSpecDecl"), \
                        [&] (const MatchFinder::MatchResult& result) mutable \
                        { \
                            if (const clang::AccessSpecDecl* decl = result.Nodes.getNodeAs<clang::AccessSpecDecl>("accessSpecDecl")) \
                            { \
                                accessSpecifiers.emplace_back(*decl, AccessSpecifier::Type::ACCESS); \
                            } \
                        }, &decl)

                    ADD_MATCHER(Public);
                    ADD_MATCHER(Protected);
                    ADD_MATCHER(Private);
                #undef ADD_MATCHER

                    // get constructors
                    const std::size_t numConstructors = std::distance(decl.ctor_begin(), decl.ctor_end());
                    constructors.reserve(numConstructors);
                #define ADD_MATCHER(ACCESS) \
                    matcher.addMatcher(cxxConstructorDecl(is ## ACCESS()).bind("constructorDecl"), \
                        [&] (const MatchFinder::MatchResult& result) mutable \
                        { \
                            if (const clang::CXXConstructorDecl* decl = result.Nodes.getNodeAs<clang::CXXConstructorDecl>("constructorDecl")) \
                            { \
                                constructors.emplace_back(*decl, AccessSpecifier::Type::ACCESS); \
                                ptr ## ACCESS ## Constructors.emplace_back(&constructors.back()); \
                                hasCopyConstructor |= constructors.back().isCopyConstructor; \
                            } \
                        }, &decl)

                    ADD_MATCHER(Public);
                    ADD_MATCHER(Protected);
                    ADD_MATCHER(Private);
                #undef ADD_MATCHER
                    
                    matcher.run(decl.getASTContext());

                    // proxy class candidate?
                    testIfProxyClassIsCandidate();

                    // do this only if is is a proxy class candidate
                    if (isProxyClassCandidate)
                    {
                        // set up access pointer
                        for (auto& access : accessSpecifiers)
                        {
                            if (ptrPublicAccess == nullptr && access.type == AccessSpecifier::Type::Public)
                            {
                                ptrPublicAccess = std::unique_ptr<AccessSpecifier>(&access);
                            }
                            else if (ptrProtectedAccess == nullptr && access.type == AccessSpecifier::Type::Protected)
                            {
                                ptrProtectedAccess = std::unique_ptr<AccessSpecifier>(&access);
                            }
                            else if (ptrPrivateAccess == nullptr && access.type == AccessSpecifier::Type::Private)
                            {
                                ptrPrivateAccess = std::unique_ptr<AccessSpecifier>(&access);
                            }
                        }

                        // find copy constructor if there is any
                        if (hasCopyConstructor)
                        {
                            for (auto& constructor : constructors)
                            {
                                if (constructor.isCopyConstructor)
                                {
                                    copyConstructor = std::unique_ptr<const Constructor>(&constructor);
                                    break;
                                }
                            }
                        }
                    }
                }

                void printInfo(clang::SourceManager& sm, const std::string indent = "") const
                {
                    std::cout << indent << "* DEFINITION:" << std::endl;
                    std::cout << indent << "\t+-> proxy class candidate: " << (isProxyClassCandidate ? "yes" : "no") << std::endl;
                    std::cout << indent << "\t+-> range: " << sourceRange.printToString(sm) << std::endl;
                    std::cout << indent << "\t+-> declaration: " << declaration.sourceRange.printToString(sm) << std::endl;
                    std::cout << indent << "\t+-> is template (partial) specialization: " << (isTemplatePartialSpecialization ? "yes" : "no") << std::endl;
                    std::cout << indent << "\t+-> fields: (public/protected/private)=(" << ptrPublicFields.size() << "/" << ptrProtectedFields.size() << "/" << ptrPrivateFields.size() << ")" << std::endl;
                    for (auto& field : fields)
                    {
                        field.printInfo(sm, indent + "\t|\t");
                    }
                    std::cout << indent << "\t+-> access specifier:" << std::endl;
                    for (auto& specifier : accessSpecifiers)
                    {
                        specifier.printInfo(sm, indent + "\t|\t");
                    }
                    std::cout << indent << "\t+-> constructor:" << std::endl;
                    for (auto& constructor : constructors)
                    {
                        constructor.printInfo(sm, indent + "\t\t");
                    }
                }
            };

        public:

            const std::string name;
            bool containsProxyClassCandidates;
            
            ClassMetaData(const std::string name)
                :
                name(name),
                containsProxyClassCandidates(false)
            { ; }

            virtual bool isTemplated() const = 0;

            virtual bool addDefinition(const clang::CXXRecordDecl& decl, const bool isTemplatePartialSpecialization = false) = 0;

            virtual const Declaration& getDeclaration() const = 0;

            virtual const std::vector<Definition>& getDefinitions() const = 0;

            virtual const clang::SourceLocation getSourceLocation() const = 0;

            virtual void printInfo(clang::SourceManager& sm, const std::string indent = "") const = 0;
        };

        class CXXClassMetaData : public ClassMetaData
        {
            using Base = ClassMetaData;

            const Base::Declaration declaration;
            std::vector<Base::Definition> definitions;

        public:

            CXXClassMetaData(const clang::CXXRecordDecl& decl, const bool isDefinition)
                :
                Base(decl.getNameAsString()),
                declaration(decl, isDefinition)
            {
                ;
            }

            virtual bool isTemplated() const
            {
                return false;
            }

            virtual bool addDefinition(const clang::CXXRecordDecl& decl, const bool isTemplatePartialSpecialization = false)
            {
                const std::string className = decl.getNameAsString();
                if (className != name) return false;

                for (auto& definition : definitions)
                {
                    // already registered
                    if (decl.getSourceRange() == definition.sourceRange) return true;
                }

                // there can be only one definition for a CXXRecordDecl
                if (definitions.size() > 0)
                {
                    std::cerr << "error in CXXClassMetaData::addDefinition() : definition is already there" << std::endl;
                }

                definitions.emplace_back(declaration, decl, isTemplatePartialSpecialization);
                containsProxyClassCandidates |= definitions.back().isProxyClassCandidate;

                return true;
            }

            virtual const Declaration& getDeclaration() const
            {
                return declaration;
            }

            virtual const std::vector<Base::Definition>& getDefinitions() const
            {
                return definitions;
            }

            virtual const clang::SourceLocation getSourceLocation() const
            {
                return declaration.sourceRange.getBegin();
            }

            virtual void printInfo(clang::SourceManager& sm, const std::string indent = "") const
            {
                std::cout << indent << "C++ " << (declaration.isClass ? "class: " : "struct: ") << name << std::endl;
                declaration.printInfo(sm, indent + "\t");
                for (auto& definition : definitions)
                {
                    definition.printInfo(sm, indent + "\t");
                }
            }
        };

        class TemplateClassMetaData : public ClassMetaData
        {
            using Base = ClassMetaData;

            const Base::Declaration declaration;
            std::vector<Base::Definition> definitions;

        public:

            TemplateClassMetaData(const clang::ClassTemplateDecl& decl, const bool isDefinition)
                :
                Base(decl.getNameAsString()),
                declaration(decl, isDefinition)
            {
                ;
            }

            virtual bool isTemplated() const
            {
                return true;
            }

            virtual bool addDefinition(const clang::CXXRecordDecl& decl, const bool isTemplatePartialSpecialization = false)
            {
                const std::string className = decl.getNameAsString();
                if (className != name) return false;

                clang::ClassTemplateDecl* classTemplateDecl = decl.getDescribedClassTemplate();
                clang::SourceRange sourceRange = (classTemplateDecl != nullptr ? classTemplateDecl->getSourceRange() : decl.getSourceRange());

                for (auto& definition : definitions)
                {
                    // already registered
                    if (sourceRange == definition.sourceRange) return true;
                }

                definitions.emplace_back(declaration, decl, isTemplatePartialSpecialization);
                containsProxyClassCandidates |= definitions.back().isProxyClassCandidate;

                return true;
            }

            virtual const Declaration& getDeclaration() const
            {
                return declaration;
            }

            virtual const std::vector<Base::Definition>& getDefinitions() const
            {
                return definitions;
            }

            virtual const clang::SourceLocation getSourceLocation() const
            {
                return declaration.sourceRange.getBegin();
            }

            virtual void printInfo(clang::SourceManager& sm, const std::string indent = "") const
            {
                std::cout << indent << "C++ template " << (declaration.isClass ? "class: " : "struct: ") << name << std::endl;
                declaration.printInfo(sm, indent + "\t");
                for (auto& definition : definitions)
                {
                    definition.printInfo(sm, indent + "\t");
                }
            }
        };
    }
}

#endif