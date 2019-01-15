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
        public:

            using SourceRangeSet = std::set<clang::SourceRange, CompareSourceRange>;

        private:

            class AccessSpecifier
            {
                clang::SourceLocation determineScopeBegin() const
                {
                    const clang::SourceLocation colonLocation = decl.getColonLoc();
                    const clang::SourceLocation colonNextLine = getNextLine(colonLocation, context);
                    const std::string colonUntilEOL = dumpSourceRangeToString(clang::SourceRange(colonLocation.getLocWithOffset(1), colonNextLine.getLocWithOffset(-1)), sourceManager);

                    if (colonUntilEOL != std::string("") && colonUntilEOL.find_first_not_of(std::string(" ")) != std::string::npos)
                    {
                        return colonLocation.getLocWithOffset(1);
                    }
                    else
                    {
                        return colonNextLine;
                    }
                }

                const clang::AccessSpecDecl& decl;
                clang::ASTContext& context;
                const clang::SourceManager& sourceManager;
                
            public:

                enum class Kind {Public = 0, Protected = 1, Private = 2};

                const clang::SourceRange sourceRange;
                const clang::SourceLocation scopeBegin;
                const Kind kind;
                const std::string kindName;

                AccessSpecifier(const clang::AccessSpecDecl& decl, Kind kind)
                    :
                    decl(decl),
                    context(decl.getASTContext()),
                    sourceManager(context.getSourceManager()),
                    sourceRange(decl.getSourceRange()),
                    scopeBegin(determineScopeBegin()),
                    kind(kind),
                    kindName(std::string(kind == Kind::Public ? "public" : (kind == Kind::Protected ? "protected" : "private")))
                { ; }

                void printInfo(const std::string indent = std::string("")) const
                {
                    std::cout << indent << "* " << kindName << std::endl;
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
                const AccessSpecifier::Kind access;
                const bool isPublic;
                const bool isProtected;
                const bool isPrivate;
                const bool isConst;
                const bool isTemplateTypeParmType;
                const bool isFundamentalOrTemplated;
                
                Field(const clang::FieldDecl& decl, const AccessSpecifier::Kind access)
                    :
                    decl(decl),
                    sourceRange(decl.getSourceRange()),
                    name(decl.getNameAsString()),
                    type(decl.getType().getTypePtrOrNull()),
                    typeName(decl.getType().getAsString()),
                    access(access),
                    isPublic(access == AccessSpecifier::Kind::Public),
                    isProtected(access == AccessSpecifier::Kind::Protected),
                    isPrivate(access == AccessSpecifier::Kind::Private),
                    isConst(decl.getType().isConstQualified()),
                    isTemplateTypeParmType(type ? type->isTemplateTypeParmType() : false),
                    isFundamentalOrTemplated(type ? (type->isFundamentalType() || isTemplateTypeParmType) : false)
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
                const AccessSpecifier::Kind access;
                const bool isPublic;
                const bool isProtected;
                const bool isPrivate;

                Constructor(const clang::CXXConstructorDecl& decl, const AccessSpecifier::Kind access)
                    :
                    decl(decl),
                    sourceRange(decl.getSourceRange()),
                    isDefaultConstructor(decl.isDefaultConstructor()),
                    isCopyConstructor(decl.isCopyConstructor()),
                    access(access),
                    isPublic(access == AccessSpecifier::Kind::Public),
                    isProtected(access == AccessSpecifier::Kind::Protected),
                    isPrivate(access == AccessSpecifier::Kind::Private)
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

                static std::vector<std::pair<std::string, bool>> getPartialSpecializationArguments(const clang::CXXRecordDecl& decl, const clang::SourceManager& sourceManager, const std::vector<const TemplateParameter*>& typeParameters)
                {
                    std::vector<std::pair<std::string, bool>> argNames;                   

                    if (const clang::ClassTemplatePartialSpecializationDecl* const tpsDecl = reinterpret_cast<const clang::ClassTemplatePartialSpecializationDecl* const>(&decl))
                    {
                        const clang::TemplateArgumentList& args = tpsDecl->getTemplateArgs();

                        for (std::size_t i = 0, iMax = args.size(); i < iMax; ++i)
                        {
                            std::string argName("\"TemplateParameter::getPartialSpecializationArguments() needs to be extended\"");                            
                            bool argHasTemplateTypeParmType = false;

                            if (args[i].getKind() == clang::TemplateArgument::ArgKind::Type)
                            {
                                const clang::QualType argQualType = args[i].getAsType();
                                argName = argQualType.getAsString();

                                if (const clang::Type* const argType = argQualType.getTypePtrOrNull())
                                {
                                    if (argHasTemplateTypeParmType = argType->isTemplateTypeParmType())
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

                            argNames.push_back(std::make_pair(argName, argHasTemplateTypeParmType));
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

        protected:

            class Namespace
            {
            public:
            
                const clang::NamespaceDecl& decl;
                const clang::SourceRange sourceRange;
                const std::string name;
                const clang::SourceLocation scopeBegin;
                const clang::SourceLocation scopeEnd;

                Namespace(const clang::NamespaceDecl& decl)
                    :
                    decl(decl),
                    sourceRange(decl.getSourceRange()),
                    name(decl.getNameAsString()),
                    scopeBegin(getLocationOfFirstOccurence(sourceRange, decl.getASTContext(), '{')),
                    scopeEnd(decl.getRBraceLoc())
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
                    std::cout << indent << "\t+-> scope begin: " << scopeBegin.printToString(sourceManager) << std::endl;
                    std::cout << indent << "\t+-> scope end: " << scopeEnd.printToString(sourceManager) << std::endl;
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
                const clang::FileID fileId;
                const clang::SourceLocation beginOfContainingFile;
                const clang::SourceLocation endOfContainingFile;
                const std::string name;
                const clang::SourceRange nameSourceRange;
                const std::vector<Namespace> namespaces;
                const std::string namespaceString;
                const std::vector<TemplateParameter> templateParameters;
                const std::vector<const TemplateParameter*> templateTypeParameters;
                const clang::SourceRange templateParameterListSourceRange;
                const bool isDefinition;
                const bool isStruct;
                const bool isClass;
                const Indentation indent;
                
                template <typename T>
                Declaration(const T& decl, const bool isDefinition)
                    :
                    cxxRecordDecl(ClassDecl::getTemplatedDecl(decl)), // is never nullptr
                    classTemplateDecl(ClassDecl::getDescribedClassTemplate(decl)), // can be nullptr
                    context(cxxRecordDecl->getASTContext()),
                    sourceManager(context.getSourceManager()),
                    sourceRange(decl.getSourceRange()),
                    fileId(sourceManager.getFileID(decl.getLocation())),
                    beginOfContainingFile(sourceManager.getLocForStartOfFile(fileId)),
                    endOfContainingFile(sourceManager.getLocForEndOfFile(fileId)),
                    nameSourceRange(decl.getLocation()),
                    name(decl.getNameAsString()),
                    namespaces(Namespace::getFromDecl(decl)),
                    namespaceString(concat(getNamespaceNames(), std::string("::")) + std::string("::")),
                    templateParameters(TemplateParameter::getParametersFromDecl(classTemplateDecl, sourceManager)),
                    templateTypeParameters(getTemplateTypeParameters()),
                    templateParameterListSourceRange(TemplateParameter::getParameterListSourceRange(classTemplateDecl)),
                    isDefinition(isDefinition),
                    isStruct(cxxRecordDecl->isStruct()),
                    isClass(cxxRecordDecl->isClass()),
                    indent(decl, namespaces.size() ? (context.getFullLoc(sourceRange.getBegin()).getSpellingColumnNumber() - 1) / namespaces.size() : 0)
                { ; }

                clang::ASTContext& getASTContext() const
                {
                    return context;
                }

                const clang::SourceManager& getSourceManager() const
                {
                    return sourceManager;
                }

                const clang::CXXRecordDecl& getCXXRecordDecl() const
                {
                    return *cxxRecordDecl;
                }

                std::vector<std::string> getTemplateParameterNames(const std::string typeParameterPrefix = std::string("")) const
                {
                    std::vector<std::string> templateParameterNames;

                    for (const auto& parameter : templateParameters)
                    {
                        if (parameter.isTypeParameter)
                        {
                            templateParameterNames.push_back(typeParameterPrefix + parameter.name);
                        }
                        else
                        {
                            templateParameterNames.push_back(parameter.name);
                        }
                    }

                    return templateParameterNames;
                }

                std::vector<std::string> getNamespaceNames() const
                {
                    std::vector<std::string> namespaceNames;

                    for (const auto& thisNamespace : namespaces)
                    {
                        namespaceNames.push_back(thisNamespace.name);
                    }

                    return namespaceNames;
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

                clang::SourceRange determineInnerSourceRange(const clang::TagDecl& decl) const
                {
                    const clang::SourceRange sourceRange = decl.getBraceRange();
                    const std::uint32_t lineLBrace = context.getFullLoc(sourceRange.getBegin()).getSpellingLineNumber();
                    const std::uint32_t lineRBrace = context.getFullLoc(sourceRange.getEnd()).getSpellingLineNumber();

                    if (lineLBrace == lineRBrace)
                    {
                        return clang::SourceRange(sourceRange.getBegin().getLocWithOffset(1), sourceRange.getEnd());
                    }
                    else
                    {
                        return clang::SourceRange(sourceManager.translateLineCol(declaration.fileId, lineLBrace + 1, 1), sourceRange.getEnd());
                    }
                }

                const clang::SourceManager& sourceManager;
                clang::ASTContext& context;
                const clang::ClassTemplateDecl* const classTemplateDecl;

            public:
                   
                const Declaration& declaration;
                const clang::CXXRecordDecl& decl;
                const clang::SourceRange sourceRange;
                const clang::SourceLocation innerLocBegin;
                const clang::SourceLocation innerLocEnd;
                const clang::SourceRange innerSourceRange;
                const std::string name;   
                const clang::SourceRange nameSourceRange;             
                const bool isTemplatePartialSpecialization;
                const std::vector<std::pair<std::string, bool>> templatePartialSpecializationArguments;          
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
                const Indentation indent;

            public:

                Definition(const Declaration& declaration, const clang::CXXRecordDecl& decl, const bool isTemplatePartialSpecialization = false)
                    :
                    sourceManager(declaration.getSourceManager()),
                    context(declaration.getASTContext()),
                    classTemplateDecl(ClassDecl::getDescribedClassTemplate(decl)), // can be nullptr
                    declaration(declaration),
                    decl(decl),
                    sourceRange(classTemplateDecl ? classTemplateDecl->getSourceRange() : decl.getSourceRange()),
                    innerLocBegin(decl.getBraceRange().getBegin().getLocWithOffset(1)),
                    innerLocEnd(decl.getBraceRange().getEnd()),
                    innerSourceRange(determineInnerSourceRange(decl)),
                    name(decl.getNameAsString()),
                    nameSourceRange(decl.getLocation()),
                    isTemplatePartialSpecialization(isTemplatePartialSpecialization),
                    templatePartialSpecializationArguments(isTemplatePartialSpecialization ? TemplateParameter::getPartialSpecializationArguments(decl, sourceManager, declaration.templateTypeParameters) : std::vector<std::pair<std::string, bool>>()),
                    publicAccess(nullptr),
                    protectedAccess(nullptr),
                    privateAccess(nullptr),
                    hasCopyConstructor(decl.hasUserDeclaredCopyConstructor()),
                    isProxyClassCandidate(true),
                    indent(decl, declaration.indent.increment)
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
                                fields.emplace_back(*decl, AccessSpecifier::Kind::ACCESS); \
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
                                accessSpecifiers.emplace_back(*decl, AccessSpecifier::Kind::ACCESS); \
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
                                constructors.emplace_back(*decl, AccessSpecifier::Kind::ACCESS); \
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
                            if (!publicAccess && access.kind == AccessSpecifier::Kind::Public)
                            {
                                publicAccess = &access;
                            }
                            else if (!protectedAccess && access.kind == AccessSpecifier::Kind::Protected)
                            {
                                protectedAccess = &access;
                            }
                            else if (!privateAccess && access.kind == AccessSpecifier::Kind::Private)
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

                std::vector<std::string> getTemplatePartialSpecializationArgumentNames(const std::string typeParameterPrefix = std::string("")) const
                {
                    std::vector<std::string> templateArguments;

                    for (const auto& argument : templatePartialSpecializationArguments)
                    {
                        if (argument.second)
                        {
                            templateArguments.push_back(typeParameterPrefix + argument.first);
                        }
                        else
                        {
                            templateArguments.push_back(argument.first);
                        }
                    }

                    return templateArguments;
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
                            std::cout << argument.first << (++argumentId < numArgument ? ", " : ">");
                        }
                        std::cout << std::endl;
                    }
                    
                    if (accessSpecifiers.size() > 0)
                    {
                        std::cout << indent << "\t+-> access specifier:" << std::endl;
                        for (const auto& specifier : accessSpecifiers)
                        {
                            specifier.printInfo(indent + std::string("\t|\t"));
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

            clang::ASTContext& context;
            const clang::SourceManager& sourceManager;
            const clang::FileID fileId;
            const std::string name;
            bool containsProxyClassCandidates;
            clang::SourceLocation topMostSourceLocation;
            clang::SourceLocation bottomMostSourceLocation;
            SourceRangeSet relevantSourceRanges;
            
        protected:

            template <typename T>
            ClassMetaData(const T& decl)
                :          
                context(decl.getASTContext()),
                sourceManager(context.getSourceManager()),
                fileId(sourceManager.getFileID(decl.getLocation())),
                name(decl.getNameAsString()),
                containsProxyClassCandidates(false),
                topMostSourceLocation(sourceManager.getLocForEndOfFile(fileId)),
                bottomMostSourceLocation(sourceManager.getLocForStartOfFile(fileId))
            { ; }



        public:

            virtual bool isTemplated() const = 0;

            virtual bool addDefinition(const clang::CXXRecordDecl& decl, const bool isTemplatePartialSpecialization = false) = 0;

            virtual const Declaration& getDeclaration() const = 0;

            virtual const std::vector<Definition>& getDefinitions() const = 0;

            virtual void printInfo(const std::string indent = std::string("")) const = 0;
        };

        class CXXClassMetaData : public ClassMetaData
        {
            using Base = ClassMetaData;

            void adaptSourceRangeInformation(const clang::SourceRange& sourceRange)
            {
                topMostSourceLocation = std::min(topMostSourceLocation, sourceRange.getBegin());
                bottomMostSourceLocation = std::max(bottomMostSourceLocation, sourceRange.getEnd());
                relevantSourceRanges.insert(sourceRange);
            }

            void determineRelevantSourceRanges()
            {
                // namespace source ranges
                for (const Namespace& thisNamespace : declaration.namespaces)
                {
                    // decl + opening (L) brace
                    const clang::SourceRange declUntilLBrace(getBeginOfLine(thisNamespace.sourceRange.getBegin(), context), thisNamespace.scopeBegin);
                    adaptSourceRangeInformation(declUntilLBrace);
                    
                    // closing brace: detect if there is just spaces before the brace (if yes, take the whole line)
                    const clang::SourceRange everythingUntilRBrace(getBeginOfLine(thisNamespace.scopeEnd, context), thisNamespace.scopeEnd);
                    const bool onlySpacesBeforeRBrace = (dumpSourceRangeToString(everythingUntilRBrace, sourceManager).find_first_not_of(std::string(" ")) == std::string::npos);
                    const clang::SourceRange untilRBrace(onlySpacesBeforeRBrace ? everythingUntilRBrace : clang::SourceRange(thisNamespace.scopeEnd));
                    adaptSourceRangeInformation(untilRBrace);
                }

                // declaration itself
                if (!declaration.isDefinition)
                {
                    const clang::SourceLocation colonLocation = getLocationOfFirstOccurence(declaration.sourceRange, context, ';');
                    const clang::SourceRange declarationWithoutIndentation(getBeginOfLine(declaration.sourceRange.getBegin(), context), colonLocation);
                    adaptSourceRangeInformation(declarationWithoutIndentation);
                }
            }

        protected:

            const Base::Declaration declaration;
            std::vector<Base::Definition> definitions;

            CXXClassMetaData(const clang::ClassTemplateDecl& decl, const bool isDefinition)
                :
                Base(decl),
                declaration(decl, isDefinition)
            {
                determineRelevantSourceRanges();
            }

            bool addDefintionKernel(const clang::CXXRecordDecl& decl, const clang::SourceRange& sourceRange, const bool isTemplatePartialSpecialization = false)
            {
                if (decl.getNameAsString() != name) return false;

                // definition already registered?
                for (const auto& definition : definitions)
                {
                    if (sourceRange == definition.sourceRange) return true;
                }

                // no, it is not: then add it!
                definitions.emplace_back(declaration, decl, isTemplatePartialSpecialization);
                
                // is this definition a proxy candidate?
                const Definition& definition = definitions.back();
                if (definition.isProxyClassCandidate)
                {
                    // there is at least one proxy class candidate
                    containsProxyClassCandidates = true;

                    // add definition source range
                    const clang::SourceRange extendedDefinitionRange = getSourceRangeWithClosingCharacter(definition.sourceRange, ';', context, false);
                    adaptSourceRangeInformation(extendedDefinitionRange);

                    return true;
                }
            
                // no, it is not: then remove it!    
                definitions.pop_back();

                return false;
            }

        public:

            CXXClassMetaData(const clang::CXXRecordDecl& decl, const bool isDefinition)
                :
                Base(decl),
                declaration(decl, isDefinition)
            {
                determineRelevantSourceRanges();
            }

            virtual bool isTemplated() const
            {
                return false;
            }

            virtual const Declaration& getDeclaration() const
            {
                return declaration;
            }

            virtual bool addDefinition(const clang::CXXRecordDecl& decl, const bool isTemplatePartialSpecialization = false)
            {
                // there can be only one definition for a CXXRecordDecl
                if (definitions.size() > 0)
                {
                    std::cerr << "CXXClassMetaData::addDefinition() : definition is already there" << std::endl;
                    return false;
                }

                return addDefintionKernel(decl, decl.getSourceRange(), isTemplatePartialSpecialization);
            }

            virtual const std::vector<Base::Definition>& getDefinitions() const
            {
                return definitions;
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
            { ; }

            virtual bool isTemplated() const
            {
                return true;
            }

            virtual bool addDefinition(const clang::CXXRecordDecl& decl, const bool isTemplatePartialSpecialization = false)
            {
                const clang::ClassTemplateDecl* const classTemplateDecl = decl.getDescribedClassTemplate();
                
                return addDefintionKernel(decl, (classTemplateDecl ? classTemplateDecl->getSourceRange() : decl.getSourceRange()), isTemplatePartialSpecialization);
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