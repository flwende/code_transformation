// Copyright (c) 2017-2018 Florian Wende (flwende@gmail.com)
//
// Distributed under the BSD 2-clause Software License
// (See accompanying file LICENSE)

#if !defined(TRAFO_DATA_LAYOUT_CLASS_META_DATA_HPP)
#define TRAFO_DATA_LAYOUT_CLASS_META_DATA_HPP

#include <iostream>
#include <cstdint>
#include <vector>
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
                    name(std::string(type == Type::Public ? "public" : (type == Type::Protected ? "protected" : "private")))
                { ; }

                void printInfo(const clang::SourceManager& sourceManager, const std::string indent = std::string("")) const
                {
                    std::cout << indent << "* " << name << std::endl;
                    std::cout << indent << "\t+-> range: " << sourceRange.printToString(sourceManager) << std::endl;
                    std::cout << indent << "\t+-> scope begin: " << scopeBegin.printToString(sourceManager) << std::endl;
                }
            };

            class Field
            {
            public:
                            
                const clang::FieldDecl& decl;
                const clang::SourceRange sourceRange;
                const std::string name;
                const clang::Type* const type;
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
                    isFundamentalOrTemplated(type ? (type->isFundamentalType() || type->isTemplateTypeParmType()) : false)
                { ; }

                void printInfo(const clang::SourceManager& sourceManager, const std::string indent = std::string("")) const
                {
                    std::cout << indent << "* name=" << name << ", type=" << typeName << (isConst ? " (const qualified)" : "") << std::endl;
                    std::cout << indent << "\t+-> range: " << sourceRange.printToString(sourceManager) << std::endl;
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
                    access(access),
                    isPublic(access == AccessSpecifier::Type::Public),
                    isProtected(access == AccessSpecifier::Type::Protected),
                    isPrivate(access == AccessSpecifier::Type::Private)
                { ; }

                void printInfo(const clang::SourceManager& sourceManager, const std::string indent = std::string("")) const
                {
                    std::cout << indent << "* " << (isDefaultConstructor ? "default " : (isCopyConstructor ? "copy " : "")) << "constructor" << std::endl;
                    std::cout << indent << "\t+-> range: " << sourceRange.printToString(sourceManager) << std::endl;
                    std::cout << indent << "\t+-> access: " << (isPublic ? "public" : (isProtected ? "protected" : "private")) << std::endl;
                    std::cout << indent << "\t+-> has body: " << (decl.hasBody() ? "yes" : "no") << std::endl;
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

                static clang::SourceRange getParameterListSourceRange(const clang::ClassTemplateDecl* const decl)
                {
                    clang::SourceRange sourceRange;

                    if (!decl) return sourceRange;

                    if (const clang::TemplateParameterList* const parameterList = decl->getTemplateParameters())
                    {
                        sourceRange = parameterList->getSourceRange();
                    }
                    
                    return sourceRange;
                }

                static std::vector<TemplateParameter> getParametersFromDecl(const clang::ClassTemplateDecl* const decl, const clang::SourceManager& sourceManager)
                {
                    std::vector<TemplateParameter> templateParameters;

                    if (!decl) return templateParameters;

                    if (const clang::TemplateParameterList* const parameterList = decl->getTemplateParameters())
                    {
                        for (std::size_t i = 0, iMax = parameterList->size(); i < iMax; ++i)
                        {
                            const clang::NamedDecl& decl = *(parameterList->getParam(i));
                            const std::string typeName = removeSpaces(dumpSourceRangeToString(decl.getSourceRange(), sourceManager));

                            templateParameters.emplace_back(decl, typeName);
                        }
                    }

                    return templateParameters;
                }

                static std::vector<std::string> getPartialSpecializationArguments(const clang::CXXRecordDecl& decl, const clang::SourceManager& sourceManager, const std::vector<const TemplateParameter*>& typeParameters)
                {
                    std::vector<std::string> argNames;                   

                    if (const clang::ClassTemplatePartialSpecializationDecl* const tpsDecl = reinterpret_cast<const clang::ClassTemplatePartialSpecializationDecl* const>(&decl))
                    {
                        const clang::TemplateArgumentList& args = tpsDecl->getTemplateArgs();

                        for (std::size_t i = 0, iMax = args.size(); i < iMax; ++i)
                        {
                            std::string argName("\"TemplateParameter::getPartialSpecializationArguments() needs to be extended\"");                            

                            if (args[i].getKind() == clang::TemplateArgument::ArgKind::Type)
                            {
                                const clang::QualType argQualType = args[i].getAsType();
                                argName = argQualType.getAsString();

                                if (const clang::Type* const argType = argQualType.getTypePtrOrNull())
                                {
                                    if (argType->isTemplateTypeParmType())
                                    {
                                        const std::uint32_t argNameTypeId = std::atoi(argName.substr(argName.rfind('-') + 1).c_str());

                                        if (argNameTypeId < typeParameters.size())
                                        {
                                            argName = typeParameters[argNameTypeId]->name;
                                        }
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

                void printInfo(const clang::SourceManager& sourceManager, const std::string indent = std::string("")) const
                {
                    std::cout << indent << "* name=" << name << ", " << (isTypeParameter ? "typename" : "value : ") << (isTypeParameter ? "" : typeName) << std::endl;
                    std::cout << indent << "\t+-> range: " << sourceRange.printToString(sourceManager) << std::endl;
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

                    Matcher matcher;
                    std::vector<Namespace> namespaces;
        
                    matcher.addMatcher(namespaceDecl(hasDescendant(namedDecl(hasName(decl.getNameAsString())))).bind("namespaceDecl"),
                        [&namespaces] (const MatchFinder::MatchResult& result) mutable
                        {
                            if (const clang::NamespaceDecl* const decl = result.Nodes.getNodeAs<clang::NamespaceDecl>("namespaceDecl"))
                            {
                                namespaces.emplace_back(*decl);
                            }
                        });

                    matcher.run(decl.getASTContext());

                    return namespaces;
                }

                void printInfo(const clang::SourceManager& sourceManager, const std::string indent = std::string("")) const
                {
                    std::cout << indent << "* " << name << ", " << sourceRange.printToString(sourceManager) << std::endl;
                }
            };

        public:

            class Declaration
            {
                std::vector<const TemplateParameter*> getTemplateTypeParameters() const
                {
                    std::vector<const TemplateParameter*> templateTypeParameters;

                    for (const auto& parameter : templateParameters)
                    {
                        if (parameter.isTypeParameter)
                        {
                            templateTypeParameters.push_back(&parameter);
                        }
                    }

                    return templateTypeParameters;
                }

                const clang::CXXRecordDecl* const cxxRecordDecl;
                const clang::ClassTemplateDecl* const classTemplateDecl;
                clang::ASTContext& context;
                const clang::SourceManager& sourceManager;

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
                Declaration(const T& decl, const bool isDefinition)
                    :
                    cxxRecordDecl(ClassDecl::getTemplatedDecl(decl)), // is never nullptr
                    classTemplateDecl(ClassDecl::getDescribedClassTemplate(decl)), // can be nullptr
                    context(cxxRecordDecl->getASTContext()),
                    sourceManager(context.getSourceManager()),
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

                clang::ASTContext& getASTContext() const
                {
                    return context;
                }

                const clang::SourceManager& getSourceManager() const
                {
                    return sourceManager;
                }

                std::vector<std::string> getTemplateParameterNames() const
                {
                    std::vector<std::string> templateParameterNames;

                    for (const auto& parameter : templateParameters)
                    {
                        templateParameterNames.push_back(parameter.name);
                    }

                    return templateParameterNames;
                }

                void printInfo(const std::string indent = std::string("")) const
                {
                    std::cout << indent << "* DECLARATION:" << std::endl;
                    std::cout << indent << "\t+-> range: " << sourceRange.printToString(sourceManager) << std::endl;

                    if (namespaces.size() > 0)
                    {
                        std::cout << indent << "\t+-> namespace(s):" << std::endl;
                        for (const auto& thisNamespace : namespaces)
                        {
                            thisNamespace.printInfo(sourceManager, indent + std::string("\t|\t"));
                        }
                    }

                    if (templateParameters.size() > 0)
                    {
                        std::cout << indent << "\t+-> template parameter list range: " << templateParameterListSourceRange.printToString(sourceManager) << std::endl;
                        for (const auto& parameter : templateParameters)
                        {
                            parameter.printInfo(sourceManager, indent + std::string("\t|\t"));
                        }
                    }

                    std::cout << indent << "\t+-> is definition: " << (isDefinition ? "yes" : "no") << std::endl;
                }
            };

            class Definition
            {
                void testIfProxyClassIsCandidate()
                {
                    bool hasMultiplePublicFieldTypes = false;
                    bool hasNonFundamentalPublicFields = false;
                    const std::string publicFieldTypeName = ptrPublicFields[0]->typeName;

                    // not abstract, polymorphic, empty
                    isProxyClassCandidate &= !(decl.isAbstract() || decl.isPolymorphic() || decl.isEmpty());

                    // at least one public field
                    isProxyClassCandidate &= (ptrPublicFields.size() > 0);

                    // public fields should be all fundamental or templated and of the same type
                    for (const auto& field : ptrPublicFields)
                    {
                        hasMultiplePublicFieldTypes |= (field->typeName != publicFieldTypeName);
                        hasNonFundamentalPublicFields |= !(field->isFundamentalOrTemplated);
                    }

                    isProxyClassCandidate &= !(hasMultiplePublicFieldTypes || hasNonFundamentalPublicFields);
                }

                const clang::SourceManager& sourceManager;
                const clang::ClassTemplateDecl* const classTemplateDecl;

            public:
                   
                const Declaration& declaration;
                const clang::CXXRecordDecl& decl;
                const clang::SourceRange sourceRange;
                const clang::SourceLocation innerLocBegin;
                const clang::SourceLocation innerLocEnd;
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
                    sourceManager(declaration.getSourceManager()),
                    classTemplateDecl(ClassDecl::getDescribedClassTemplate(decl)), // can be nullptr
                    declaration(declaration),
                    decl(decl),
                    sourceRange(classTemplateDecl ? classTemplateDecl->getSourceRange() : decl.getSourceRange()),
                    innerLocBegin(decl.getBraceRange().getBegin().getLocWithOffset(1)),
                    innerLocEnd(decl.getBraceRange().getEnd()),
                    name(decl.getNameAsString()),
                    isTemplatePartialSpecialization(isTemplatePartialSpecialization),
                    templatePartialSpecializationArguments(isTemplatePartialSpecialization ? TemplateParameter::getPartialSpecializationArguments(decl, sourceManager, declaration.templateTypeParameters) : std::vector<std::string>()),
                    publicAccess(nullptr),
                    protectedAccess(nullptr),
                    privateAccess(nullptr),
                    hasCopyConstructor(decl.hasUserDeclaredCopyConstructor()),
                    isProxyClassCandidate(true)
                {
                    using namespace clang::ast_matchers;

                    Matcher matcher;
                    
                    // get fields
                    const std::size_t numFields = std::distance(decl.field_begin(), decl.field_end());
                    fields.reserve(numFields);
                #define ADD_MATCHER(ACCESS) \
                    matcher.addMatcher(fieldDecl(is ## ACCESS()).bind("fieldDecl"), \
                        [this] (const MatchFinder::MatchResult& result) mutable \
                        { \
                            if (const clang::FieldDecl* const decl = result.Nodes.getNodeAs<clang::FieldDecl>("fieldDecl")) \
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
                        [this] (const MatchFinder::MatchResult& result) mutable \
                        { \
                            if (const clang::AccessSpecDecl* const decl = result.Nodes.getNodeAs<clang::AccessSpecDecl>("accessSpecDecl")) \
                            { \
                                accessSpecifiers.emplace_back(*decl, AccessSpecifier::Type::ACCESS); \
                            } \
                        }, &decl)

                    ADD_MATCHER(Public);
                    ADD_MATCHER(Protected);
                    ADD_MATCHER(Private);
                #undef ADD_MATCHER

                    // get user provided constructors
                    const std::size_t numConstructors = std::distance(decl.ctor_begin(), decl.ctor_end());
                    constructors.reserve(numConstructors);
                #define ADD_MATCHER(ACCESS) \
                    matcher.addMatcher(cxxConstructorDecl(allOf(is ## ACCESS(), isUserProvided())).bind("constructorDecl"), \
                        [this] (const MatchFinder::MatchResult& result) mutable \
                        { \
                            if (const clang::CXXConstructorDecl* const decl = result.Nodes.getNodeAs<clang::CXXConstructorDecl>("constructorDecl")) \
                            { \
                                constructors.emplace_back(*decl, AccessSpecifier::Type::ACCESS); \
                                ptr ## ACCESS ## Constructors.emplace_back(&constructors.back()); \
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
                        for (const auto& access : accessSpecifiers)
                        {
                            if (!publicAccess && access.type == AccessSpecifier::Type::Public)
                            {
                                publicAccess = &access;
                            }
                            else if (!protectedAccess && access.type == AccessSpecifier::Type::Protected)
                            {
                                protectedAccess = &access;
                            }
                            else if (!privateAccess && access.type == AccessSpecifier::Type::Private)
                            {
                                privateAccess = &access;
                            }
                        }

                        // find copy constructor if there is any
                        if (hasCopyConstructor)
                        {
                            for (const auto& constructor : constructors)
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

                const clang::SourceManager& getSourceManager() const
                {
                    return declaration.getSourceManager();
                }

                void printInfo(const std::string indent = std::string("")) const
                {
                    std::cout << indent << "* DEFINITION:" << std::endl;
                    std::cout << indent << "\t+-> range: " << sourceRange.printToString(sourceManager) << std::endl;
                    std::cout << indent << "\t+-> declaration: " << declaration.sourceRange.printToString(sourceManager) << std::endl;
                    std::cout << indent << "\t+-> is template (partial) specialization: " << (isTemplatePartialSpecialization ? "yes" : "no") << std::endl;
                    if (isTemplatePartialSpecialization)
                    {
                        const std::uint32_t numArgument = templatePartialSpecializationArguments.size();
                        std::uint32_t argumentId = 0;

                        std::cout << indent << "\t|\t* arguments: <";
                        for (const auto& argument : templatePartialSpecializationArguments)
                        {
                            std::cout << argument << (++argumentId < numArgument ? ", " : ">");
                        }
                        std::cout << std::endl;
                    }
                    
                    if (accessSpecifiers.size() > 0)
                    {
                        std::cout << indent << "\t+-> access specifier:" << std::endl;
                        for (const auto& specifier : accessSpecifiers)
                        {
                            specifier.printInfo(sourceManager, indent + std::string("\t|\t"));
                        }
                    }

                    if (constructors.size() > 0)
                    {
                        std::cout << indent << "\t+-> constructor:" << std::endl;
                        for (const auto& constructor : constructors)
                        {
                            constructor.printInfo(sourceManager, indent + std::string("\t|\t"));
                        }
                    }

                    if (fields.size() > 0)
                    {
                        std::cout << indent << "\t+-> fields: (public/protected/private)=(" << ptrPublicFields.size() << "/" << ptrProtectedFields.size() << "/" << ptrPrivateFields.size() << ")" << std::endl;
                        for (const auto& field : fields)
                        {
                            field.printInfo(sourceManager, indent + std::string("\t\t"));
                        }
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

            virtual clang::ASTContext& getASTContext() const = 0;

            virtual const clang::SourceManager& getSourceManager() const = 0;
            
            virtual bool isTemplated() const = 0;

            virtual bool addDefinition(const clang::CXXRecordDecl& decl, const bool isTemplatePartialSpecialization = false) = 0;

            virtual const Declaration& getDeclaration() const = 0;

            virtual const std::vector<Definition>& getDefinitions() const = 0;

            virtual const clang::SourceLocation getSourceLocation() const = 0;

            virtual void printInfo(const std::string indent = std::string("")) const = 0;
        };

        class CXXClassMetaData : public ClassMetaData
        {
            using Base = ClassMetaData;

        protected:

            const Base::Declaration declaration;
            std::vector<Base::Definition> definitions;

            CXXClassMetaData(const clang::ClassTemplateDecl& decl, const bool isDefinition)
                :
                Base(decl.getNameAsString()),
                declaration(decl, isDefinition)
            {
                ;
            }

        public:

            CXXClassMetaData(const clang::CXXRecordDecl& decl, const bool isDefinition)
                :
                Base(decl.getNameAsString()),
                declaration(decl, isDefinition)
            {
                ;
            }

            virtual clang::ASTContext& getASTContext() const
            {
                return declaration.getASTContext();
            }

            virtual const clang::SourceManager& getSourceManager() const
            {
                return declaration.getSourceManager();
            }

            virtual bool isTemplated() const
            {
                return false;
            }

            virtual bool addDefinition(const clang::CXXRecordDecl& decl, const bool isTemplatePartialSpecialization = false)
            {
                const std::string className = decl.getNameAsString();

                if (className != name) return false;

                for (const auto& definition : definitions)
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

            virtual void printInfo(const std::string indent = std::string("")) const
            {
                std::cout << indent << "C++ " << (declaration.isClass ? "class: " : "struct: ") << name << std::endl;
                declaration.printInfo(indent + std::string("\t"));
                for (const auto& definition : definitions)
                {
                    definition.printInfo(indent + std::string("\t"));
                }
            }
        };

        class TemplateClassMetaData : public CXXClassMetaData
        {
            using Base = CXXClassMetaData;
            using Base::declaration;
            using Base::definitions;

        public:

            TemplateClassMetaData(const clang::ClassTemplateDecl& decl, const bool isDefinition)
                :
                Base(decl, isDefinition)
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

                const clang::ClassTemplateDecl* const classTemplateDecl = decl.getDescribedClassTemplate();
                const clang::SourceRange sourceRange = (classTemplateDecl ? classTemplateDecl->getSourceRange() : decl.getSourceRange());

                for (const auto& definition : definitions)
                {
                    // already registered
                    if (sourceRange == definition.sourceRange) return true;
                }

                definitions.emplace_back(declaration, decl, isTemplatePartialSpecialization);
                containsProxyClassCandidates |= definitions.back().isProxyClassCandidate;

                return true;
            }

            virtual void printInfo(const std::string indent = std::string("")) const
            {
                std::cout << indent << "C++ template " << (declaration.isClass ? "class: " : "struct: ") << name << std::endl;
                declaration.printInfo(indent + std::string("\t"));
                for (const auto& definition : definitions)
                {
                    definition.printInfo(indent + std::string("\t"));
                }
            }
        };
    }
}

#endif