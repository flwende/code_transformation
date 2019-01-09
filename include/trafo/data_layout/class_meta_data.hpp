// Copyright (c) 2017-2018 Florian Wende (flwende@gmail.com)
//
// Distributed under the BSD 2-clause Software License
// (See accompanying file LICENSE)

#if !defined(TRAFO_DATA_LAYOUT_CLASS_META_DATA_HPP)
#define TRAFO_DATA_LAYOUT_CLASS_META_DATA_HPP

#include <iostream>
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
                const clang::Type* type;
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

            class TemplateParameter
            {
            public:

                const clang::NamedDecl& decl;
                const clang::SourceRange sourceRange;
                const std::string name;
                const std::string typeName;
                const bool isTypeParameter;

                TemplateParameter(const clang::NamedDecl& decl, const std::string typeName)
                    :
                    decl(decl),
                    sourceRange(decl.getSourceRange()),
                    name(decl.getNameAsString()),
                    typeName(typeName),
                    isTypeParameter(typeName == std::string("typename") || typeName == std::string("class"))
                { ; }

                static clang::SourceRange getParameterListSourceRange(const clang::ClassTemplateDecl* decl)
                {
                    clang::SourceRange sourceRange;

                    if (decl == nullptr) return sourceRange;

                    if (const clang::TemplateParameterList* parameterList = decl->getTemplateParameters())
                    {
                        sourceRange = parameterList->getSourceRange();
                    }

                    return sourceRange;
                }

                static std::vector<TemplateParameter> getParametersFromDecl(const clang::ClassTemplateDecl* decl, clang::SourceManager& sm)
                {
                    std::vector<TemplateParameter> templateParameters;

                    if (decl == nullptr) return templateParameters;

                    if (const clang::TemplateParameterList* parameterList = decl->getTemplateParameters())
                    {
                        const std::size_t numParameters = parameterList->size();
                        for (std::size_t i = 0; i < numParameters; ++i)
                        {
                            const clang::NamedDecl& decl = *(parameterList->getParam(i));
                            templateParameters.emplace_back(decl, removeSpaces(dumpSourceRangeToString(decl.getSourceRange(), sm)));
                        }
                    }

                    return templateParameters;
                }

                static std::vector<std::string> getPartialSpecializationArguments(const clang::CXXRecordDecl& decl, clang::SourceManager& sm, const std::vector<const TemplateParameter*>& typeParameters)
                {
                    std::vector<std::string> argNames;
                    
                    if (const clang::ClassTemplatePartialSpecializationDecl* tpsDecl = reinterpret_cast<const clang::ClassTemplatePartialSpecializationDecl*>(&decl))
                    {
                        const clang::TemplateArgumentList& args = tpsDecl->getTemplateArgs();
                        for (std::size_t i = 0, iMax = args.size(); i < iMax; ++i)
                        {
                            std::string argName("\"TemplateParameter::getPartialSpecializationArguments() needs to be extended\"");
                            
                            if (args[i].getKind() == clang::TemplateArgument::ArgKind::Type)
                            {
                                const clang::QualType argQualType = args[i].getAsType();
                                const clang::Type* argType = argQualType.getTypePtrOrNull();
                                argName = argQualType.getAsString();
                                if (argType != nullptr && argType->isTemplateTypeParmType())
                                {
                                    const std::uint32_t argNameTypeId = std::atoi(argName.substr(argName.rfind('-') + 1).c_str());
                                    if (argNameTypeId < typeParameters.size())
                                    {
                                        argName = typeParameters[argNameTypeId]->name;
                                    }
                                }
                            }
                            else if (args[i].getKind() == clang::TemplateArgument::ArgKind::Integral)
                            {
                                argName = std::to_string(args[i].getAsIntegral().getExtValue());
                            }
                            
                            argNames.push_back(argName);
                        }   
                    }

                    return argNames;
                }

                void printInfo(clang::SourceManager& sm, const std::string indent = "") const
                {
                    std::cout << indent << "* name=" << name << ", " << (isTypeParameter ? "typename" : "value : ") << (isTypeParameter ? "" : typeName) << std::endl;
                    std::cout << indent << "\t+-> range: " << sourceRange.printToString(sm) << std::endl;
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
                clang::SourceManager& sourceManager;
                const clang::CXXRecordDecl* cxxRecordDecl;
                const clang::ClassTemplateDecl* classTemplateDecl;

                std::vector<const TemplateParameter*> getTemplateTypeParameters() const
                {
                    std::vector<const TemplateParameter*> templateTypeParameters;
                    for (auto& parameter : templateParameters)
                    {
                        if (parameter.isTypeParameter)
                        {
                            templateTypeParameters.push_back(&parameter);
                        }
                    }
                    return templateTypeParameters;
                }

            public:
        
                const clang::SourceRange sourceRange;
                const std::string name;
                const std::vector<Namespace> namespaces;
                const std::vector<TemplateParameter> templateParameters;
                const std::vector<const TemplateParameter*> templateTypeParameters;
                const clang::SourceRange templateParameterListSourceRange;
                const bool isDefinition;
                const bool isStruct;
                const bool isClass;

                template <typename T>
                Declaration(const T& decl, const bool isDefinition, clang::SourceManager& sourceManager)
                    :
                    sourceManager(sourceManager),
                    cxxRecordDecl(ClassDecl::getTemplatedDecl(decl)), // is never nullptr
                    classTemplateDecl(ClassDecl::getDescribedClassTemplate(decl)), // can be nullptr
                    sourceRange(decl.getSourceRange()),
                    name(decl.getNameAsString()),
                    namespaces(Namespace::getFromDecl(decl)),
                    templateParameters(TemplateParameter::getParametersFromDecl(classTemplateDecl, sourceManager)),
                    templateTypeParameters(getTemplateTypeParameters()),
                    templateParameterListSourceRange(TemplateParameter::getParameterListSourceRange(classTemplateDecl)),
                    isDefinition(isDefinition),
                    isStruct(cxxRecordDecl->isStruct()),
                    isClass(cxxRecordDecl->isClass())
                { ; }

                clang::SourceManager& getSourceMgr() const
                {
                    return sourceManager;
                }

                std::vector<std::string> getTemplateParameterNames() const
                {
                    std::vector<std::string> templateParameterNames;
                    for (auto& parameter : templateParameters)
                    {
                        templateParameterNames.push_back(parameter.name);
                    }
                    return templateParameterNames;
                }

                void printInfo(const std::string indent = "") const
                {
                    std::cout << indent << "* DECLARATION:" << std::endl;
                    std::cout << indent << "\t+-> range: " << sourceRange.printToString(sourceManager) << std::endl;
                    if (namespaces.size() > 0)
                    {
                        std::cout << indent << "\t+-> namespace(s):" << std::endl;
                        for (auto& thisNamespace : namespaces)
                        {
                            thisNamespace.printInfo(sourceManager, indent + "\t|\t");
                        }
                    }
                    if (templateParameters.size() > 0)
                    {
                        std::cout << indent << "\t+-> template parameter list range: " << templateParameterListSourceRange.printToString(sourceManager) << std::endl;
                        for (auto& parameter : templateParameters)
                        {
                            parameter.printInfo(sourceManager, indent + "\t|\t");
                        }
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

                clang::SourceManager& sourceManager;
                const clang::ClassTemplateDecl* classTemplateDecl;

            public:
            
                
                const Declaration& declaration;
                const clang::CXXRecordDecl& decl;
                const clang::SourceRange sourceRange;
                const std::string name;                
                const bool isTemplatePartialSpecialization;
                const std::vector<std::string> templatePartialSpecializationArguments;            
                std::vector<Field> fields;
                std::vector<const Field*> ptrPublicFields;
                std::vector<const Field*> ptrProtectedFields;
                std::vector<const Field*> ptrPrivateFields;
                std::vector<AccessSpecifier> accessSpecifiers;
                const AccessSpecifier* publicAccess;
                const AccessSpecifier* protectedAccess;
                const AccessSpecifier* privateAccess;
                std::vector<Constructor> constructors;
                std::vector<const Constructor*> ptrPublicConstructors;
                std::vector<const Constructor*> ptrProtectedConstructors;
                std::vector<const Constructor*> ptrPrivateConstructors;
                bool hasCopyConstructor;
                const Constructor* copyConstructor;
                bool isProxyClassCandidate;

                Definition(const Declaration& declaration, const clang::CXXRecordDecl& decl, const bool isTemplatePartialSpecialization = false)
                    :
                    sourceManager(declaration.getSourceMgr()),
                    classTemplateDecl(ClassDecl::getDescribedClassTemplate(decl)), // can be nullptr
                    declaration(declaration),
                    decl(decl),
                    sourceRange(classTemplateDecl ? classTemplateDecl->getSourceRange() : decl.getSourceRange()),
                    name(decl.getNameAsString()),
                    isTemplatePartialSpecialization(isTemplatePartialSpecialization),
                    templatePartialSpecializationArguments(isTemplatePartialSpecialization ? TemplateParameter::getPartialSpecializationArguments(decl, sourceManager, declaration.templateTypeParameters) : std::vector<std::string>()),
                    publicAccess(nullptr),
                    protectedAccess(nullptr),
                    privateAccess(nullptr),
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
                            if (publicAccess == nullptr && access.type == AccessSpecifier::Type::Public)
                            {
                                publicAccess = &access;
                            }
                            else if (protectedAccess == nullptr && access.type == AccessSpecifier::Type::Protected)
                            {
                                protectedAccess = &access;
                            }
                            else if (privateAccess == nullptr && access.type == AccessSpecifier::Type::Private)
                            {
                                privateAccess = &access;
                            }
                        }

                        // find copy constructor if there is any
                        if (hasCopyConstructor)
                        {
                            for (auto& constructor : constructors)
                            {
                                if (constructor.isCopyConstructor)
                                {
                                    copyConstructor = &constructor;
                                    break;
                                }
                            }
                        }
                    }
                }

                void printInfo(const std::string indent = "") const
                {
                    std::cout << indent << "* DEFINITION:" << std::endl;
                    std::cout << indent << "\t+-> range: " << sourceRange.printToString(sourceManager) << std::endl;
                    std::cout << indent << "\t+-> declaration: " << declaration.sourceRange.printToString(sourceManager) << std::endl;
                    std::cout << indent << "\t+-> is template (partial) specialization: " << (isTemplatePartialSpecialization ? "yes" : "no") << std::endl;
                    if (isTemplatePartialSpecialization)
                    {
                        std::cout << indent << "\t|\t* arguments: <";
                        const std::uint32_t numArgument = templatePartialSpecializationArguments.size();
                        std::uint32_t argumentId = 0;
                        for (auto argument : templatePartialSpecializationArguments)
                        {
                            std::cout << argument << (++argumentId < numArgument ? ", " : ">");
                        }
                        std::cout << std::endl;
                    }
                    std::cout << indent << "\t+-> access specifier:" << std::endl;
                    for (auto& specifier : accessSpecifiers)
                    {
                        specifier.printInfo(sourceManager, indent + "\t|\t");
                    }
                    std::cout << indent << "\t+-> constructor:" << std::endl;
                    for (auto& constructor : constructors)
                    {
                        constructor.printInfo(sourceManager, indent + "\t|\t");
                    }
                    std::cout << indent << "\t+-> fields: (public/protected/private)=(" << ptrPublicFields.size() << "/" << ptrProtectedFields.size() << "/" << ptrPrivateFields.size() << ")" << std::endl;
                    for (auto& field : fields)
                    {
                        field.printInfo(sourceManager, indent + "\t\t");
                    }
                }
            };

        protected:

            clang::SourceManager& sourceManager;

        public:

            const std::string name;
            bool containsProxyClassCandidates;
            
            ClassMetaData(const std::string name, clang::SourceManager& sourceManager)
                :
                sourceManager(sourceManager),                
                name(name),
                containsProxyClassCandidates(false)
            { ; }

            virtual bool isTemplated() const = 0;

            virtual bool addDefinition(const clang::CXXRecordDecl& decl, const bool isTemplatePartialSpecialization = false) = 0;

            virtual const Declaration& getDeclaration() const = 0;

            virtual const std::vector<Definition>& getDefinitions() const = 0;

            virtual const clang::SourceLocation getSourceLocation() const = 0;

            virtual void printInfo(const std::string indent = "") const = 0;
        };

        class CXXClassMetaData : public ClassMetaData
        {
            using Base = ClassMetaData;

            const Base::Declaration declaration;
            std::vector<Base::Definition> definitions;

        public:

            CXXClassMetaData(const clang::CXXRecordDecl& decl, const bool isDefinition, clang::SourceManager& sourceManager)
                :
                Base(decl.getNameAsString(), sourceManager),
                declaration(decl, isDefinition, sourceManager)
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

            virtual void printInfo(const std::string indent = "") const
            {
                std::cout << indent << "C++ " << (declaration.isClass ? "class: " : "struct: ") << name << std::endl;
                declaration.printInfo(indent + "\t");
                for (auto& definition : definitions)
                {
                    definition.printInfo(indent + "\t");
                }
            }
        };

        class TemplateClassMetaData : public ClassMetaData
        {
            using Base = ClassMetaData;

            const Base::Declaration declaration;
            std::vector<Base::Definition> definitions;

        public:

            TemplateClassMetaData(const clang::ClassTemplateDecl& decl, const bool isDefinition, clang::SourceManager& sourceManager)
                :
                Base(decl.getNameAsString(), sourceManager),
                declaration(decl, isDefinition, sourceManager)
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

            virtual void printInfo(const std::string indent = "") const
            {
                std::cout << indent << "C++ template " << (declaration.isClass ? "class: " : "struct: ") << name << std::endl;
                declaration.printInfo(indent + "\t");
                for (auto& definition : definitions)
                {
                    definition.printInfo(indent + "\t");
                }
            }
        };
    }
}

#endif