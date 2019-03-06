// Copyright (c) 2017-2018 Florian Wende (flwende@gmail.com)
//
// Distributed under the BSD 2-clause Software License
// (See accompanying file LICENSE)

#if !defined(TRAFO_DATA_LAYOUT_CLASS_META_DATA_HPP)
#define TRAFO_DATA_LAYOUT_CLASS_META_DATA_HPP

#include <iostream>
#include <cstdint>
#include <memory>
#include <numeric>
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

            // forward declaration
            class Definition;

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

            // forward declaration
            class CXXMethod;

            class FunctionArgument
            {
                clang::SourceRange getFunctionArgumentSourceRange(const clang::ParmVarDecl& decl)
                {
                    return getSpellingSourceRange(decl.getSourceRange(), decl.getASTContext());
                }

                bool isConstQualified(const clang::ParmVarDecl& decl)
                {
                    const clang::QualType qualType = decl.getType();

                    if (qualType.isConstQualified()) return true;

                    return (qualType.getAsString().find("const", 0, 5) != std::string::npos);
                }

                std::string getTypeName(const clang::ParmVarDecl& decl, const Definition* definition = nullptr, const bool getElementTypeName = false)
                {
                    const clang::QualType qualType = decl.getType();
                    std::string typeName = (getElementTypeName ? qualType.getUnqualifiedType().getNonReferenceType().getAsString() : qualType.getAsString());

                    if (definition)
                    {
                        const std::string className = definition->name + definition->templateParameterString;
                        const std::string classNameInternal = definition->name + definition->templateParameterStringInternal;

                        if (!findAndReplace(typeName, className, definition->name))
                        {
                            findAndReplace(typeName, classNameInternal, definition->name);
                        }
                    }

                    // in some cases the 'getUnqualifiedType()' method does not work properly: remove cvr explicitly
                    if (getElementTypeName)
                    {
                        findAndReplace(typeName, std::string("const"), std::string(""), true, true);
                        findAndReplace(typeName, std::string("volatile"), std::string(""), true, true);
                        findAndReplace(typeName, std::string("__restrict__"), std::string(""), true, true);
                        typeName = removeSpaces(typeName);
                    }

                    return typeName;
                }

            public:

                const clang::ParmVarDecl& decl;
                const clang::SourceRange sourceRange;
                const std::string name;
                const std::string typeName;
                const std::string elementTypeName;
                const bool isConst;
                const bool isClassType;
                const bool isLReference;
                const bool isRReference;
                const bool isPointer;

                FunctionArgument(const clang::ParmVarDecl& decl, const Definition* definition = nullptr)
                    :
                    decl(decl),
                    sourceRange(getSpellingSourceRange(decl.getSourceRange(), decl.getASTContext())),
                    name(decl.getNameAsString()),
                    typeName(getTypeName(decl, definition)),
                    elementTypeName(getTypeName(decl, definition, true)),
                    isConst(isConstQualified(decl)),
                    isClassType(definition ? (typeName.find(definition->name) != std::string::npos) : false),
                    isLReference(decl.getType()->isLValueReferenceType()),
                    isRReference(decl.getType()->isRValueReferenceType()),
                    isPointer(decl.getType()->isPointerType())
                { ; }

                static std::vector<FunctionArgument> getArgumentsFromDecl(const clang::FunctionDecl& decl, const Definition* definition = nullptr)
                {
                    std::vector<FunctionArgument> arguments;

                    for (const auto param : decl.parameters())
                    {
                        arguments.emplace_back(*param, definition);
                    }

                    return arguments;
                }

                void printInfo(const clang::SourceManager& sourceManager, const std::string indent = std::string("")) const
                {
                    std::cout << indent << "* name=" << name << ", type=" << typeName << (isConst ? " (const qualified) " : "");
                    std::cout << ", element type=" << elementTypeName << std::endl;
                    std::cout << indent << "\t+-> range: " << sourceRange.printToString(sourceManager) << std::endl;
                }
            };

            class CXXMethod
            {
                friend class FunctionArgument;

                std::string getClassName(const clang::CXXMethodDecl& decl, const Definition* definition = nullptr)
                {
                    if (definition)
                    {
                        return definition->name;
                    }
                    else if (decl.getParent())
                    {
                        return decl.getParent()->getNameAsString();
                    }

                    return std::string("");
                }

                std::vector<std::uint32_t> getClassTypeArguments()
                {
                    std::vector<std::uint32_t> classTypeArguments;
                    
                    for (std::uint32_t i = 0; i < arguments.size(); ++i)
                    {
                        if (arguments[i].isClassType)
                        {
                            classTypeArguments.push_back(i);
                        }
                    }
                    
                    return classTypeArguments;
                }

            public:

                const clang::FunctionDecl& decl;
                const clang::CXXMethodDecl* const cxxMethodDecl;
                const clang::FunctionTemplateDecl* const functionTemplateDecl;
                const clang::SourceRange sourceRange;
                const clang::SourceRange extendedSourceRange;
                const std::string name;
                const std::string className;
                const bool isFunctionTemplate;
                const bool isMacroExpansion;
                const clang::SourceRange macroDefinitionSourceRange;
                const std::vector<FunctionArgument> arguments;
                const bool hasClassTypeArguments;
                const clang::QualType returnType;
                const clang::SourceRange returnTypeSourceRange;
                const std::string returnTypeName;
                const bool returnsClassType;

                CXXMethod(const clang::CXXMethodDecl& decl, const Definition* definition = nullptr)
                    :
                    decl(decl),
                    cxxMethodDecl(&decl),
                    functionTemplateDecl(nullptr),
                    sourceRange(decl.getSourceRange()),
                    extendedSourceRange(extendSourceRangeByLines(sourceRange, 1, decl.getASTContext())),
                    name(getNameBeforeMacroExpansion(decl, ClassMetaData::preprocessor)),
                    className(getClassName(decl, definition)),
                    isFunctionTemplate(false),
                    isMacroExpansion(isThisAMacroExpansion(decl)),
                    macroDefinitionSourceRange(getSpellingSourceRange(sourceRange, decl.getASTContext())),
                    arguments(FunctionArgument::getArgumentsFromDecl(decl, definition)),
                    hasClassTypeArguments(std::accumulate(arguments.begin(), arguments.end(), false, [] (const bool red, const FunctionArgument& argument) { return (red | argument.isClassType); })),
                    returnType(decl.getReturnType()),
                    returnTypeSourceRange(decl.isNoReturn() ? clang::SourceRange() : decl.getReturnTypeSourceRange()),
                    returnTypeName(decl.isNoReturn() ? std::string("") : dumpSourceRangeToString(returnTypeSourceRange, decl.getASTContext().getSourceManager())),
                    returnsClassType(decl.isNoReturn() ? false : (returnTypeName == className))
                { ; }

                CXXMethod(const clang::FunctionDecl& decl, const std::string className = std::string(""), const Definition* definition = nullptr, const bool isFunctionTemplateDecl = false)
                    :
                    decl(decl),
                    cxxMethodDecl(nullptr),
                    functionTemplateDecl(isFunctionTemplateDecl ? decl.getDescribedFunctionTemplate() : nullptr),
                    sourceRange(isFunctionTemplateDecl ? functionTemplateDecl->getSourceRange() : decl.getSourceRange()),
                    extendedSourceRange(extendSourceRangeByLines(sourceRange, 1, decl.getASTContext())),
                    name(getNameBeforeMacroExpansion(decl, ClassMetaData::preprocessor)),
                    className(className),
                    isFunctionTemplate(isFunctionTemplateDecl),
                    isMacroExpansion(isThisAMacroExpansion(decl)),
                    macroDefinitionSourceRange(getSpellingSourceRange(sourceRange, decl.getASTContext())),
                    arguments(FunctionArgument::getArgumentsFromDecl(decl, definition)),
                    hasClassTypeArguments(std::accumulate(arguments.begin(), arguments.end(), false, [] (const bool red, const FunctionArgument& argument) { return (red | argument.isClassType); })),
                    returnType(decl.getReturnType()),
                    returnTypeSourceRange(decl.isNoReturn() ? clang::SourceRange() : decl.getReturnTypeSourceRange()),
                    returnTypeName(decl.isNoReturn() ? std::string("") : dumpSourceRangeToString(returnTypeSourceRange, decl.getASTContext().getSourceManager())),
                    returnsClassType(decl.isNoReturn() ? false : (returnTypeName == className))
                { ; }

                void printInfo(const clang::SourceManager& sourceManager, const std::string indent = std::string("")) const
                {
                    std::cout << indent << "* name=" << name << " (of class " << className << ")" << std::endl;
                    std::cout << indent << "\t+-> range: " << sourceRange.printToString(sourceManager) << std::endl;
                    std::cout << indent << "\t+-> extended range: " << extendedSourceRange.printToString(sourceManager) << std::endl;
                    if (isMacroExpansion)
                    {
                        std::cout << indent << "\t+-> macro definition range: " << macroDefinitionSourceRange.printToString(sourceManager) << std::endl;
                    }

                    std::cout << indent << "\t+-> returns class type: " << (returnsClassType ? "yes" : "no") << std::endl;
                    std::cout << indent << "\t\t* source range: " << returnTypeSourceRange.printToString(sourceManager) << std::endl;
                    
                    if (arguments.size())
                    {
                        std::cout << indent << "\t+-> arguments:" << std::endl;
                        std::cout << indent << "\t\t* has class type arguments: " << (hasClassTypeArguments ? "yes" : "no") << std::endl;
                        for (const auto& argument : arguments)
                        {
                            argument.printInfo(sourceManager, indent + std::string("\t\t"));
                        }
                    }
                }
            };

            class TemplateParameter
            {
                static std::string getArgumentName(const clang::TemplateArgument& arg, const std::vector<TemplateParameter>& templateParameters, const bool internalRepresentation = false, bool* isTypeParameter = nullptr)
                {
                    std::string argName("ERROR");
                    
                    if (isTypeParameter)
                    {
                        *isTypeParameter = false;
                    }

                    if (arg.getKind() == clang::TemplateArgument::ArgKind::Type)
                    {
                        const clang::QualType argQualType = arg.getAsType();
                        argName = argQualType.getAsString();

                        if (!internalRepresentation)
                        {
                            if (const clang::Type* const argType = argQualType.getTypePtrOrNull())
                            {
                                if (argType->isTemplateTypeParmType())
                                {
                                    const std::uint32_t argNameTypeId = std::atoi(argName.substr(argName.rfind('-') + 1).c_str());

                                    for (std::size_t i = 0, t = 0; i < templateParameters.size(); ++i)
                                    {
                                        if (templateParameters[i].isTypeParameter)
                                        {
                                            if (argNameTypeId == t)
                                            {
                                                argName = templateParameters[i].name;
                                                break;
                                            }
                                            ++t;
                                        }
                                    }
                                    
                                    if (isTypeParameter)
                                    {
                                        *isTypeParameter = true;
                                    }
                                }
                            }
                        }
                    }
                    else if (arg.getKind() == clang::TemplateArgument::ArgKind::Integral)
                    {
                        argName = std::to_string(arg.getAsIntegral().getExtValue());
                    }

                    return argName;
                };

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
                            const clang::NamedDecl& namedDecl = *(parameterList->getParam(i));
                            const std::string typeName = removeSpaces(dumpSourceRangeToString(namedDecl.getSourceRange(), sourceManager));

                            templateParameters.emplace_back(namedDecl, typeName);
                        }
                    }

                    return templateParameters;
                }

                static std::string getPartialSpecializationArgumentString(const clang::CXXRecordDecl& decl, const std::vector<TemplateParameter>& templateParameters, const bool internalRepresentation = false)
                {
                    std::stringstream templateArgumentStringStream;

                    if (const clang::ClassTemplatePartialSpecializationDecl* const tpsDecl = reinterpret_cast<const clang::ClassTemplatePartialSpecializationDecl* const>(&decl))
                    {
                        const clang::TemplateArgumentList& args = tpsDecl->getTemplateArgs();

                        if (const std::uint32_t iMax = args.size())
                        {
                            templateArgumentStringStream << "<";
                            for (std::uint32_t i = 0; i < iMax; ++i)
                            {
                                const std::string argName = getArgumentName(args[i], templateParameters, internalRepresentation);
                                templateArgumentStringStream << argName << ((i + 1) < iMax ? ", " : ">");
                            }
                        }   
                    }

                    return templateArgumentStringStream.str();
                }

                static std::vector<std::pair<std::string, bool>> getPartialSpecializationArguments(const clang::CXXRecordDecl& decl, const std::vector<TemplateParameter>& templateParameters)
                {
                    std::vector<std::pair<std::string, bool>> argNames;                   

                    if (const clang::ClassTemplatePartialSpecializationDecl* const tpsDecl = reinterpret_cast<const clang::ClassTemplatePartialSpecializationDecl* const>(&decl))
                    {
                        const clang::TemplateArgumentList& args = tpsDecl->getTemplateArgs();
                        for (std::uint32_t i = 0, iMax = args.size(); i < iMax; ++i)
                        {
                            bool isTypeParameter = false;
                            const std::string argName = getArgumentName(args[i], templateParameters, false, &isTypeParameter);
                            argNames.push_back(std::make_pair(argName, isTypeParameter));
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
                    name(getNameBeforeMacroExpansion(decl, ClassMetaData::preprocessor)),
                    scopeBegin(getLocationOfFirstOccurence(sourceRange, decl.getASTContext(), '{')),
                    scopeEnd(decl.getRBraceLoc())
                { ; }

                static std::vector<Namespace> getNamespacesFromDecl(const clang::NamedDecl& decl)
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
                std::string getTemplateParamerString(const bool withTypeName = false, const std::string namePrefix = std::string("")) const
                {
                    std::stringstream templateParameterStringStream;
                    const std::uint32_t numTemplateParameters = templateParameters.size();
                    
                    if (numTemplateParameters > 0)
                    {
                        templateParameterStringStream << "<";
                        for (std::uint32_t i = 0; i < numTemplateParameters; ++i)
                        {
                            if (withTypeName)
                            {
                                templateParameterStringStream << templateParameters[i].typeName << " ";
                            }

                            if (templateParameters[i].isTypeParameter)
                            {
                                templateParameterStringStream << namePrefix;
                            }
    
                            templateParameterStringStream << templateParameters[i].name << ((i + 1) < numTemplateParameters ? ", " : ">");
                        }
                    }

                    return templateParameterStringStream.str();
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
                const clang::SourceRange templateParameterListSourceRange;
                const std::string templateParameterDeclString;
                const std::string templateParameterDeclStringX;
                const std::string templateParameterString;
                const std::string templateParameterStringX;
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
                    namespaces(Namespace::getNamespacesFromDecl(decl)),
                    namespaceString(concat(getNamespaceNames(), std::string("::")) + std::string("::")),
                    templateParameters(TemplateParameter::getParametersFromDecl(classTemplateDecl, sourceManager)),
                    templateParameterListSourceRange(TemplateParameter::getParameterListSourceRange(classTemplateDecl)),
                    templateParameterDeclString(getTemplateParamerString(true)),
                    templateParameterDeclStringX(getTemplateParamerString(true, std::string("_"))),
                    templateParameterString(getTemplateParamerString()),
                    templateParameterStringX(getTemplateParamerString(false, std::string("_"))),
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
                        std::cout << indent << "\t|\t* as decl string: " << templateParameterDeclString << std::endl;
                        std::cout << indent << "\t|\t* as decl string x: " << templateParameterDeclStringX << std::endl;
                        std::cout << indent << "\t|\t* as string: " << templateParameterString << std::endl;
                        std::cout << indent << "\t|\t* as string x: " << templateParameterStringX << std::endl;
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
                friend class CXXMethod;

                void testIfProxyClassIsCandidate()
                {
                    bool hasMultiplePublicFieldTypes = false;
                    bool hasNonFundamentalPublicFields = false;

                    // at least one public field
                    isProxyClassCandidate &= (numPublicFields > 0);

                    // not abstract, polymorphic, empty
                    isProxyClassCandidate &= !(decl.isAbstract() || decl.isPolymorphic() || decl.isEmpty());

                    if (!isProxyClassCandidate) return;

                    // public fields should be all fundamental or templated and of the same type
                    for (const auto& field : fields)
                    {
                        if (!field.isPublic) continue;

                        hasMultiplePublicFieldTypes |= (field.typeName != publicFieldTypeName);
                        hasNonFundamentalPublicFields |= !(field.isFundamentalOrTemplated);
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
                const std::string templateParameterString;
                const std::string templateParameterStringInternal;
                const std::vector<std::pair<std::string, bool>> templatePartialSpecializationArguments;
                std::vector<Field> fields;
                std::uint32_t numPublicFields;
                std::uint32_t numProtectedFields;
                std::uint32_t numPrivateFields;
                std::string publicFieldTypeName;
                std::vector<AccessSpecifier> accessSpecifiers;
                const AccessSpecifier* publicAccess;
                const AccessSpecifier* protectedAccess;
                const AccessSpecifier* privateAccess;
                std::vector<Constructor> constructors;
                std::vector<const Constructor*> ptrPublicConstructors;
                std::vector<const Constructor*> ptrProtectedConstructors;
                std::vector<const Constructor*> ptrPrivateConstructors;
                std::vector<CXXMethod> cxxMethods;
                bool hasCopyConstructor;
                const Constructor* copyConstructor;
                bool isProxyClassCandidate;
                bool isHomogeneous;
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
                    templateParameterString(isTemplatePartialSpecialization ? TemplateParameter::getPartialSpecializationArgumentString(decl, declaration.templateParameters) : declaration.templateParameterString),
                    templateParameterStringInternal(isTemplatePartialSpecialization ? TemplateParameter::getPartialSpecializationArgumentString(decl, declaration.templateParameters, true) : declaration.templateParameterString),
                    templatePartialSpecializationArguments(isTemplatePartialSpecialization ? TemplateParameter::getPartialSpecializationArguments(decl, declaration.templateParameters) : std::vector<std::pair<std::string, bool>>()),
                    numPublicFields(0),
                    numProtectedFields(0),
                    numPrivateFields(0),
                    publicFieldTypeName(std::string("")),
                    publicAccess(nullptr),
                    protectedAccess(nullptr),
                    privateAccess(nullptr),
                    hasCopyConstructor(decl.hasUserDeclaredCopyConstructor()),
                    isProxyClassCandidate(true),
                    isHomogeneous(true),
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
                                ++num ## ACCESS ## Fields; \
                                if (fields.back().isPublic && publicFieldTypeName.empty()) publicFieldTypeName = fields.back().typeName; \
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

                    // get templated methods
                    matcher.addMatcher(functionTemplateDecl().bind("functionTemplateDecl"),
                        [this] (const MatchFinder::MatchResult& result) mutable
                        {
                            if (const clang::FunctionTemplateDecl* const decl = result.Nodes.getNodeAs<clang::FunctionTemplateDecl>("functionTemplateDecl"))
                            {
                                if (!find(decl->getNameAsString(), name, true))
                                {
                                    if (const clang::FunctionDecl* const functionDecl = decl->getTemplatedDecl())
                                    {
                                        cxxMethods.emplace_back(*functionDecl, name, this, true);
                                    }
                                }
                            }
                        }, &decl);

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

                        // is this a homogeneous structured type?
                        isHomogeneous = std::accumulate(fields.begin(), fields.end(), true, 
                            [this] (const bool red, const Field& field) 
                            { 
                                return red & (field.typeName == fields[0].typeName); 
                            });

                        const std::string className = name + templateParameterString;
                        const std::string classNameInternal = name + templateParameterStringInternal;

                        for (const auto& method : decl.methods())
                        {
                            if (!method->isUserProvided()) continue;

                            const clang::DeclarationName::NameKind kind = method->getDeclName().getNameKind();
                            if (kind == clang::DeclarationName::NameKind::CXXConstructorName || kind == clang::DeclarationName::NameKind::CXXDestructorName) continue;

                            cxxMethods.emplace_back(*method, this);
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
                        std::cout << indent << "\t|\t* arguments: " << templateParameterString << " (internal: " << templateParameterStringInternal << ")" << std::endl;
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

                    if (cxxMethods.size() > 0)
                    {
                        std::cout << indent << "\t+-> methods:" << std::endl;
                        for (const auto& method : cxxMethods)
                        {
                            method.printInfo(sourceManager, indent + std::string("\t|\t"));
                        }
                    }

                    if (fields.size() > 0)
                    {
                        std::cout << indent << "\t+-> fields: " << (isHomogeneous ? "homogeneous type" : "inhomogeneous type") << std::endl;
                        for (const auto& field : fields)
                        {
                            field.printInfo(sourceManager, indent + std::string("\t\t"));
                        }
                    }
                }
            };

            //clang::Preprocessor& preprocessor;
            clang::ASTContext& context;
            const clang::SourceManager& sourceManager;
            const clang::FileID fileId;
            const std::string name;
            bool containsProxyClassCandidates;
            clang::SourceLocation topMostSourceLocation;
            clang::SourceLocation bottomMostSourceLocation;
            SourceRangeSet relevantSourceRanges;
        
        protected:

            static std::shared_ptr<clang::Preprocessor> preprocessor;

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

            static void registerPreprocessor(std::shared_ptr<clang::Preprocessor> preprocessor)
            {
                if (!ClassMetaData::preprocessor.get())
                {
                    ClassMetaData::preprocessor = preprocessor;
                    //std::cout << preprocessor->getPredefines() << std::endl;
                    /*
                    clang::IdentifierTable& idTab = preprocessor->getIdentifierTable();
                    for (auto it = idTab.begin(); it != idTab.end(); ++it)
                    {
                        std::cout << "IDTAB: " << it->first().str() << std::endl;
                    }
                    */
                }
            }

            virtual bool isTemplated() const = 0;

            virtual bool addDefinition(const clang::CXXRecordDecl& decl, const bool isTemplatePartialSpecialization = false) = 0;

            virtual const Declaration& getDeclaration() const = 0;

            virtual const std::vector<Definition>& getDefinitions() const = 0;

            virtual void printInfo(const std::string indent = std::string("")) const = 0;
        };

        std::shared_ptr<clang::Preprocessor> ClassMetaData::preprocessor;

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