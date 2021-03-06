// Copyright (c) 2017-2019 Florian Wende (flwende@gmail.com)
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

            // forward declaration
            class Function;
            class Constructor;
            class Field;

            class FunctionArgument
            {
                friend class Function;
                friend class Constructor;
                friend class Field;

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

                static std::string getTypeName(const clang::QualType& qualType, const Definition* definition = nullptr, const bool getElementTypeName = false)
                {
                    std::string typeName = (getElementTypeName ? qualType.getUnqualifiedType().getNonReferenceType().getAsString() : qualType.getAsString());
                    
                    if (getElementTypeName)
                    {
                        if (definition && find(typeName, definition->name))
                        {
                            typeName = definition->name;
                        }
                        else
                        {
                            // in some cases the 'getUnqualifiedType()' method does not work properly: remove cvr explicitly
                            findAndReplace(typeName, std::string("const"), std::string(""), true, true);
                            findAndReplace(typeName, std::string("volatile"), std::string(""), true, true);
                            findAndReplace(typeName, std::string("__restrict__"), std::string(""), true, true);
                            typeName = removeSpaces(typeName);
                        }
                        
                    }

                    return typeName;
                }

                static std::string getTemplateParameters(const clang::ParmVarDecl& decl, const Definition* definition = nullptr)
                {
                    const clang::QualType qualType = decl.getType();
                    const std::string typeName = qualType.getUnqualifiedType().getNonReferenceType().getAsString();

                    const std::size_t pos = typeName.find('<');
                    if (pos != std::string::npos)
                    {
                        return typeName.substr(pos);
                    }

                    return {};
                }

            public:

                const clang::ParmVarDecl& decl;
                const clang::SourceRange sourceRange;
                const std::string name;
                const std::string typeName;
                const std::string elementTypeName;
                const std::string templateParameters;
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
                    typeName(getTypeName(decl.getType(), definition)),
                    elementTypeName(getTypeName(decl.getType(), definition, true)),
                    templateParameters(getTemplateParameters(decl, definition)),
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
                    if (templateParameters.length())
                    {
                        std::cout << indent << "\t+-> template parameters=" << templateParameters << std::endl;
                    }
                    std::cout << indent << "\t+-> range: " << sourceRange.printToString(sourceManager) << std::endl;
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
                const std::string elementTypeName;
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
                    elementTypeName(decl.getType().getUnqualifiedType().getAsString()),
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

            class Function
            {
                friend class FunctionArgument;

                std::string getClassName(const clang::CXXMethodDecl& decl, const Definition* definition = nullptr) const
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

                std::vector<std::uint32_t> getClassTypeArguments() const 
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

                std::vector<clang::SourceRange> getSignature() const
                {
                    std::vector<clang::SourceRange> signature;

                    if (!isMacroExpansion)
                    {
                        signature.reserve(arguments.size() + 2);
                        signature.emplace_back(sourceRange.getBegin(), getLocationOfFirstOccurrence(sourceRange, decl.getASTContext(), name).getLocWithOffset(name.length()));

                        for (std::uint32_t i = 0; i < arguments.size(); ++i)
                        {
                            
                            if ((i + 1) < arguments.size())
                            {
                                signature.emplace_back(arguments[i].sourceRange.getBegin(), 
                                    getLocationOfLastOccurrence(clang::SourceRange(arguments[i].sourceRange.getBegin(), arguments[i + 1].sourceRange.getBegin()), decl.getASTContext(), std::string(",")));                           
                            }
                            else
                            {
                                signature.emplace_back(arguments[i].sourceRange.getBegin(), 
                                    getLocationOfLastOccurrence(clang::SourceRange(arguments[i].sourceRange.getBegin(), bodySourceRange.getBegin()), decl.getASTContext(), std::string(")")));                           
                            }
                        }

                        // skip ')': it is the character we used for the detection of the last argument
                        signature.emplace_back(signature[arguments.size()].getEnd().getLocWithOffset(1), bodySourceRange.getBegin());
                    }

                    return signature;
                }

                std::vector<std::string> getSignatureString() const
                {
                    std::vector<std::string> signatureString;

                    signatureString.reserve(signature.size());

                    for (std::uint32_t i = 0; i < signature.size(); ++i)
                    {   
                        signatureString.push_back(dumpSourceRangeToString(signature[i], decl.getASTContext().getSourceManager()));
                    }

                    return signatureString;
                }

            public:

                const clang::FunctionDecl& decl;
                const clang::CXXMethodDecl* const cxxMethodDecl;
                const clang::FunctionTemplateDecl* const functionTemplateDecl;
                const clang::SourceRange sourceRange;
                const clang::SourceRange extendedSourceRange;
                const std::string name;
                const std::string className;
                const bool isDefinition;
                const bool isConst;
                const clang::Stmt* body;
                const clang::SourceRange bodySourceRange;
                const clang::SourceRange extendedBodySourceRange;
                const clang::SourceLocation bodyInnerLocationBegin;
                const bool isFunctionTemplate;
                const bool isMacroExpansion;
                const clang::SourceRange macroDefinitionSourceRange;
                const std::vector<FunctionArgument> arguments;
                const std::vector<std::uint32_t> indexClassTypeArguments;
                const bool hasClassTypeArguments;
                const bool classTypeArgumentsHaveDefaultArg;
                const std::vector<clang::SourceRange> signature;
                const clang::SourceRange signatureSourceRange;
                const std::vector<std::string> signatureString;
                const clang::QualType returnType;
                const std::string returnTypeName;
                const std::string elementReturnTypeName;
                const clang::SourceRange returnTypeSourceRange;
                const bool returnsClassType;
                const bool returnsClassTypeReference;

                Function(const clang::CXXMethodDecl& decl, const Definition* definition = nullptr)
                    :
                    decl(decl),
                    cxxMethodDecl(&decl),
                    functionTemplateDecl(nullptr),
                    sourceRange(decl.getSourceRange()),
                    extendedSourceRange(extendSourceRangeByLines(sourceRange, 1, decl.getASTContext())),
                    name(getNameBeforeMacroExpansion(decl, ClassMetaData::preprocessor)),
                    className(getClassName(decl, definition)),
                    isDefinition(decl.isThisDeclarationADefinition()),
                    isConst(decl.isConst()),
                    body(isDefinition ? decl.getBody() : nullptr),
                    bodySourceRange(isDefinition ? body->getSourceRange() : clang::SourceRange()),
                    extendedBodySourceRange(extendSourceRangeByLines(bodySourceRange, 1, decl.getASTContext())),
                    bodyInnerLocationBegin(isDefinition ? getNextLine(body->getBeginLoc(), decl.getASTContext()) : clang::SourceLocation()), 
                    isFunctionTemplate(false),
                    isMacroExpansion(isThisAMacroExpansion(decl)),
                    macroDefinitionSourceRange(getSpellingSourceRange(sourceRange, decl.getASTContext())),
                    arguments(FunctionArgument::getArgumentsFromDecl(decl, definition)),
                    indexClassTypeArguments(getClassTypeArguments()),
                    hasClassTypeArguments(indexClassTypeArguments.size() > 0),
                    classTypeArgumentsHaveDefaultArg(std::accumulate(arguments.begin(), arguments.end(), false, 
                        [] (const bool red, const FunctionArgument& argument) 
                        { 
                            return (red | (argument.isClassType && argument.decl.hasDefaultArg()));
                        })),
                    signature(getSignature()),
                    signatureSourceRange(signature.size() > 0 ? clang::SourceRange(signature[0].getBegin() , signature[signature.size() - 1].getEnd()) : clang::SourceRange()),
                    signatureString(getSignatureString()),
                    returnType(decl.getReturnType()),
                    returnTypeName(decl.isNoReturn() ? std::string("") : FunctionArgument::getTypeName(decl.getReturnType(), definition)),
                    elementReturnTypeName(decl.isNoReturn() ? std::string("") : FunctionArgument::getTypeName(decl.getReturnType(), definition, true)),
                    returnTypeSourceRange(decl.isNoReturn() ? clang::SourceRange() : clang::SourceRange(decl.getReturnTypeSourceRange().getBegin(),
                        decl.getReturnTypeSourceRange().getBegin().getLocWithOffset(elementReturnTypeName.length() - 1))),
                    returnsClassType(decl.isNoReturn() ? false : (elementReturnTypeName == className)),
                    returnsClassTypeReference(returnsClassType ? (decl.getReturnType()->isReferenceType() || decl.getReturnType()->isPointerType()) : false)
                { ; }

                Function(const clang::FunctionDecl& decl, const std::string className = std::string(""), const Definition* definition = nullptr, const bool isFunctionTemplateDecl = false)
                    :
                    decl(decl),
                    cxxMethodDecl(nullptr),
                    functionTemplateDecl(isFunctionTemplateDecl ? decl.getDescribedFunctionTemplate() : nullptr),
                    sourceRange(isFunctionTemplateDecl ? functionTemplateDecl->getSourceRange() : decl.getSourceRange()),
                    extendedSourceRange(extendSourceRangeByLines(sourceRange, 1, decl.getASTContext())),
                    name(getNameBeforeMacroExpansion(decl, ClassMetaData::preprocessor)),
                    className(className),
                    isDefinition(decl.isThisDeclarationADefinition()),
                    isConst(false),
                    body(isDefinition ? decl.getBody() : nullptr),
                    bodySourceRange(isDefinition ? body->getSourceRange() : clang::SourceRange()),
                    extendedBodySourceRange(extendSourceRangeByLines(bodySourceRange, 1, decl.getASTContext())),
                    bodyInnerLocationBegin(isDefinition ? getNextLine(body->getBeginLoc(), decl.getASTContext()) : clang::SourceLocation()),
                    isFunctionTemplate(isFunctionTemplateDecl),
                    isMacroExpansion(isThisAMacroExpansion(decl)),
                    macroDefinitionSourceRange(getSpellingSourceRange(sourceRange, decl.getASTContext())),
                    arguments(FunctionArgument::getArgumentsFromDecl(decl, definition)),
                    indexClassTypeArguments(getClassTypeArguments()),
                    hasClassTypeArguments(indexClassTypeArguments.size() > 0),
                    classTypeArgumentsHaveDefaultArg(std::accumulate(arguments.begin(), arguments.end(), false, 
                        [] (const bool red, const FunctionArgument& argument) 
                        { 
                            return (red | (argument.isClassType && argument.decl.hasDefaultArg()));
                        })),
                    signature(getSignature()),
                    signatureSourceRange(signature.size() > 0 ? clang::SourceRange(signature[0].getBegin() , signature[signature.size() - 1].getEnd()) : clang::SourceRange()),
                    signatureString(getSignatureString()),
                    returnType(decl.getReturnType()),
                    returnTypeName(decl.isNoReturn() ? std::string("") : FunctionArgument::getTypeName(decl.getReturnType(), definition)),
                    elementReturnTypeName(decl.isNoReturn() ? std::string("") : FunctionArgument::getTypeName(decl.getReturnType(), definition, true)),
                    returnTypeSourceRange(decl.isNoReturn() ? clang::SourceRange() : clang::SourceRange(decl.getReturnTypeSourceRange().getBegin(),
                        decl.getReturnTypeSourceRange().getBegin().getLocWithOffset(elementReturnTypeName.length() - 1))),
                    returnsClassType(decl.isNoReturn() ? false : (elementReturnTypeName == className)),
                    returnsClassTypeReference(returnsClassType ? (decl.getReturnType()->isReferenceType() || decl.getReturnType()->isPointerType()) : false)
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

                    if (isDefinition)
                    {
                        std::cout << indent << "\t+-> body range: " << bodySourceRange.printToString(sourceManager) << std::endl;
                    }

                    std::cout << indent << "\t+-> return type: " << returnTypeName << " (" << elementReturnTypeName << ")" << std::endl;
                    std::cout << indent << "\t\t* returns class type: " << (returnsClassType ? "yes" : "no");
                    if (returnsClassTypeReference)
                    {
                        std::cout << " (reference/pointer)";
                    }
                    std::cout << std::endl;
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

            class Constructor
            {
                bool isFunctionCopyConstructor(const clang::FunctionDecl& decl, const Definition* definition = nullptr)
                {
                    if (decl.getNumParams() != 1) return false;

                    if (const clang::ParmVarDecl* parameter = decl.getParamDecl(0))
                    {
                        //const std::string typeName = FunctionArgument::getTypeName(*decl.getParamDecl(0), definition, true);
                        const std::string typeName = FunctionArgument::getTypeName(decl.getParamDecl(0)->getType(), definition, true);

                        return (typeName.find(definition->name) != std::string::npos);
                    }

                    return false;
                }

            public:

                const clang::FunctionDecl& decl;
                const clang::SourceRange sourceRange;                
                const bool isTemplatedConstructor;
                const clang::SourceRange templateSignatureSourceRange;
                const bool isDefaultConstructor;
                const bool isCopyConstructor;
                const AccessSpecifier::Kind access;
                const bool isPublic;
                const bool isProtected;
                const bool isPrivate;

                Constructor(const clang::FunctionDecl& decl, const AccessSpecifier::Kind access, const bool isTemplatedConstructor = false, const Definition* definition = nullptr)
                    :
                    decl(decl),
                    sourceRange(isTemplatedConstructor ? decl.getDescribedFunctionTemplate()->getSourceRange() : decl.getSourceRange()),
                    isTemplatedConstructor(isTemplatedConstructor),
                    templateSignatureSourceRange(isTemplatedConstructor ? clang::SourceRange(sourceRange.getBegin(), decl.getLocation()) : clang::SourceRange()),
                    isDefaultConstructor(isTemplatedConstructor ? false : static_cast<const clang::CXXConstructorDecl&>(decl).isDefaultConstructor()),
                    isCopyConstructor(isTemplatedConstructor ? isFunctionCopyConstructor(decl, definition) : static_cast<const clang::CXXConstructorDecl&>(decl).isCopyConstructor()),
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

                                    for (std::uint32_t i = 0, j = 0; i < templateParameters.size(); ++i)
                                    {
                                        if (templateParameters[i].isTypeParameter)
                                        {
                                            if (j == argNameTypeId)
                                            {
                                                argName = templateParameters[i].name;
                                                break;
                                            }
                                            ++j;
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

                static std::string getPartialSpecializationArgumentString(const clang::CXXRecordDecl& decl, const std::vector<TemplateParameter>& templateParameters, const bool internalRepresentation = false, const std::string namePrefix = std::string(""))
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
                                if (templateParameters[i].isTypeParameter)
                                {
                                    templateArgumentStringStream << namePrefix;
                                }

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
                    scopeBegin(getLocationOfFirstOccurrence(sourceRange, decl.getASTContext(), std::string("{"))),
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
                                const std::string sourceLocationString = decl->getBeginLoc().printToString(decl->getASTContext().getSourceManager());
                            
                                if (sourceLocationString.find("usr/lib") != std::string::npos ||
                                    sourceLocationString.find("usr/include") != std::string::npos) return;

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
                const clang::SourceRange extendedSourceRange;
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
                const std::string templateConstParameterString;
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
                    extendedSourceRange({getBeginOfLine(sourceRange.getBegin(), context), getLocationOfLastOccurrence({sourceRange.getBegin(), getNextLine(sourceRange.getEnd(), context)}, context, std::string(";"), 0, 0).getLocWithOffset(1)}),
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
                    templateParameterString(getTemplateParamerString(false)),
                    templateParameterStringX(getTemplateParamerString(false, std::string("_"))),
                    templateConstParameterString(getTemplateParamerString(false, std::string("const "))),
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
                friend class Function;

                void testIfProxyClassIsCandidate()
                {
                    bool hasNonFundamentalFields = false;

                    // not abstract, polymorphic, empty
                    isProxyClassCandidate &= !(decl.isAbstract() || decl.isPolymorphic() || decl.isEmpty());

                    // at least one public field
                    isProxyClassCandidate &= (indexPublicFields.size() > 0);

                    // public fields should be of fundamental or templated type
                    for (const auto field : fields)
                    {
                        hasNonFundamentalFields |= !(field.isFundamentalOrTemplated);
                    }

                    isProxyClassCandidate &= !hasNonFundamentalFields;
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
                const clang::SourceRange extendedSourceRange;
                const clang::SourceLocation innerLocBegin;
                const clang::SourceLocation innerLocEnd;
                const clang::SourceRange innerSourceRange;
                const std::string name;   
                const clang::SourceRange nameSourceRange;             
                const bool isTemplatePartialSpecialization;
                const std::string templateParameterString;
                const std::string templateConstParameterString;
                const std::string templateParameterStringX;
                const std::string templateParameterStringInternal;
                const std::vector<std::pair<std::string, bool>> templatePartialSpecializationArguments;
                std::vector<Field> fields;
                std::vector<std::uint32_t> indexPublicFields;
                std::vector<std::uint32_t> indexProtectedFields;
                std::vector<std::uint32_t> indexPrivateFields;                
                std::vector<AccessSpecifier> accessSpecifiers;
                std::vector<std::uint32_t> indexPublicAccessSpecifiers;
                std::vector<std::uint32_t> indexProtectedAccessSpecifiers;
                std::vector<std::uint32_t> indexPrivateAccessSpecifiers;
                std::vector<Constructor> constructors;
                std::vector<std::uint32_t> indexPublicConstructors;
                std::vector<std::uint32_t> indexProtectedConstructors;
                std::vector<std::uint32_t> indexPrivateConstructors;
                bool hasCopyConstructor;
                std::uint32_t indexCopyConstructor;
                std::vector<Function> cxxMethods;
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
                    extendedSourceRange({getBeginOfLine(sourceRange.getBegin(), context), getLocationOfLastOccurrence({sourceRange.getBegin(), getNextLine(sourceRange.getEnd(), context)}, context, std::string(";"), 0, 0).getLocWithOffset(1)}),
                    innerLocBegin(decl.getBraceRange().getBegin().getLocWithOffset(1)),
                    innerLocEnd(decl.getBraceRange().getEnd()),
                    innerSourceRange(determineInnerSourceRange(decl)),
                    name(decl.getNameAsString()),
                    nameSourceRange(decl.getLocation()),
                    isTemplatePartialSpecialization(isTemplatePartialSpecialization),
                    templateParameterString(isTemplatePartialSpecialization ? TemplateParameter::getPartialSpecializationArgumentString(decl, declaration.templateParameters) : declaration.templateParameterString),
                    templateConstParameterString(isTemplatePartialSpecialization ? TemplateParameter::getPartialSpecializationArgumentString(decl, declaration.templateParameters, false, std::string("const ")) : declaration.templateConstParameterString),
                    templateParameterStringX(isTemplatePartialSpecialization ? TemplateParameter::getPartialSpecializationArgumentString(decl, declaration.templateParameters, false, std::string("_")) : declaration.templateParameterStringX),
                    templateParameterStringInternal(isTemplatePartialSpecialization ? TemplateParameter::getPartialSpecializationArgumentString(decl, declaration.templateParameters, true) : declaration.templateParameterString),
                    templatePartialSpecializationArguments(isTemplatePartialSpecialization ? TemplateParameter::getPartialSpecializationArguments(decl, declaration.templateParameters) : std::vector<std::pair<std::string, bool>>()),
                    hasCopyConstructor(false),
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
                                index ## ACCESS ## Fields.push_back(fields.size()); \
                                fields.emplace_back(*decl, AccessSpecifier::Kind::ACCESS); \
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
                                index ## ACCESS ## AccessSpecifiers.push_back(accessSpecifiers.size()); \
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
                                index ## ACCESS ## Constructors.push_back(constructors.size()); \
                                constructors.emplace_back(*decl, AccessSpecifier::Kind::ACCESS); \
                                if (!hasCopyConstructor && constructors.back().isCopyConstructor) \
                                { \
                                    hasCopyConstructor = true; \
                                    indexCopyConstructor = index ## ACCESS ## Constructors.back(); \
                                } \
                            } \
                        }, &decl)

                    ADD_MATCHER(Public);
                    ADD_MATCHER(Protected);
                    ADD_MATCHER(Private);
                #undef ADD_MATCHER

                #define ADD_MATCHER(ACCESS) \
                    matcher.addMatcher(functionTemplateDecl(is ## ACCESS()).bind("templatedConstructorDecl"), \
                        [this] (const MatchFinder::MatchResult& result) mutable \
                        { \
                            if (const clang::FunctionTemplateDecl* const decl = result.Nodes.getNodeAs<clang::FunctionTemplateDecl>("templatedConstructorDecl")) \
                            { \
                                if (find(decl->getNameAsString(), name, true)) \
                                { \
                                    index ## ACCESS ## Constructors.push_back(constructors.size()); \
                                    constructors.emplace_back(*decl->getTemplatedDecl(), AccessSpecifier::Kind::ACCESS, true, this); \
                                    if (!hasCopyConstructor && constructors.back().isCopyConstructor) \
                                    { \
                                        hasCopyConstructor = true; \
                                        indexCopyConstructor = index ## ACCESS ## Constructors.back(); \
                                    } \
                                } \
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

                    // do this only if it is a proxy class candidate
                    if (isProxyClassCandidate)
                    {
                        // is this a homogeneous structured type?
                        isHomogeneous = std::accumulate(fields.begin(), fields.end(), true, 
                            [this] (const bool red, const Field& field) 
                            { 
                                return red & (field.typeName == fields[0].typeName); 
                            });

                        // get non-templated method declarations 
                        for (const auto& method : decl.methods())
                        {
                            // skip auto-generated methods
                            if (!method->isUserProvided()) continue;

                            // skip constructors and destructors
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
                        std::cout << indent << "\t|\t* arguments x: " << templateParameterStringX << std::endl;
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
                        std::cout << indent << "\t+-> fields: (public/protected/private)=(" << indexPublicFields.size() << "/" << indexProtectedFields.size() << "/" << indexPrivateFields.size() << ")";
                        std::cout << ", is homogeneous: " << (isHomogeneous ? "yes" : "no") << std::endl;
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
            const std::string filename;
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
                bottomMostSourceLocation(sourceManager.getLocForStartOfFile(fileId)),
                filename(sourceManager.getFilename(decl.getLocation()).str())
            { ; }

        public:

            static void registerPreprocessor(std::shared_ptr<clang::Preprocessor> preprocessor)
            {
                if (!ClassMetaData::preprocessor.get())
                {
                    ClassMetaData::preprocessor = preprocessor;
                }
            }

            virtual bool isTemplated() const = 0;

            virtual bool addDefinition(const clang::CXXRecordDecl& decl, const bool isTemplatePartialSpecialization = false, const bool isInstantiated = true) = 0;

            virtual const Declaration& getDeclaration() const = 0;

            virtual const std::vector<Definition>& getDefinitions() const = 0;

            virtual const std::vector<clang::SourceRange>& getDefinitionsSortedOut() const = 0;

            SourceRangeSet& getRelevantSourceRanges()
            {
                return relevantSourceRanges;
            }

            const SourceRangeSet& getRelevantSourceRanges() const
            {
                return relevantSourceRanges;
            }

            void updateRelevantSourceRangeInfo() 
            {
                if (relevantSourceRanges.size() > 0)
                {
                    const auto& front = relevantSourceRanges.begin();
                    const auto& back = relevantSourceRanges.rbegin();

                    topMostSourceLocation = std::min(topMostSourceLocation, front->getBegin());
                    bottomMostSourceLocation = std::max(topMostSourceLocation, back->getEnd());
                }
            }

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
                    const clang::SourceLocation colonLocation = getLocationOfFirstOccurrence(declaration.sourceRange, context, std::string(";"));
                    const clang::SourceRange declarationWithoutIndentation(getBeginOfLine(declaration.sourceRange.getBegin(), context), colonLocation);
                    adaptSourceRangeInformation(declarationWithoutIndentation);
                }
            }

        protected:

            const Base::Declaration declaration;
            std::vector<Base::Definition> definitions;
            std::vector<clang::SourceRange> definitionsSortedOut;

            CXXClassMetaData(const clang::ClassTemplateDecl& decl, const bool isDefinition)
                :
                Base(decl),
                declaration(decl, isDefinition)
            {
                determineRelevantSourceRanges();
            }

            bool addDefintionKernel(const clang::CXXRecordDecl& decl, const clang::SourceRange& sourceRange, const bool isTemplatePartialSpecialization = false, const bool isInstantiated = true)
            {
                if (decl.getNameAsString() != name) return false;

                if (!isInstantiated)
                {
                    definitionsSortedOut.push_back(decl.getSourceRange());
                    return true;
                }

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
                    const clang::SourceRange extendedDefinitionRange = getSourceRangeWithClosingCharacter(definition.sourceRange, std::string(";"), context, false);
                    adaptSourceRangeInformation(extendedDefinitionRange);

                    return true;
                }

                // no, it is not: then remove it!    
                definitions.pop_back();

                // sort it out
                definitionsSortedOut.push_back(decl.getSourceRange());

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

            virtual bool addDefinition(const clang::CXXRecordDecl& decl, const bool isTemplatePartialSpecialization = false, const bool isInstantiated = true)
            {
                // there can be only one definition for a CXXRecordDecl
                if (definitions.size() > 0)
                {
                    std::cerr << "CXXClassMetaData::addDefinition() : definition is already there" << std::endl;
                    return false;
                }

                return addDefintionKernel(decl, decl.getSourceRange(), isTemplatePartialSpecialization, isInstantiated);
            }

            virtual const std::vector<Base::Definition>& getDefinitions() const
            {
                return definitions;
            }

            const std::vector<clang::SourceRange>& getDefinitionsSortedOut() const
            {
                return definitionsSortedOut;
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
            using Base::definitionsSortedOut;

        public:

            TemplateClassMetaData(const clang::ClassTemplateDecl& decl, const bool isDefinition)
                :
                Base(decl, isDefinition)
            { ; }

            virtual bool isTemplated() const
            {
                return true;
            }

            virtual bool addDefinition(const clang::CXXRecordDecl& decl, const bool isTemplatePartialSpecialization = false, const bool isInstantiated = true)
            {
                const clang::ClassTemplateDecl* const classTemplateDecl = decl.getDescribedClassTemplate();
                
                return addDefintionKernel(decl, (classTemplateDecl ? classTemplateDecl->getSourceRange() : decl.getSourceRange()), isTemplatePartialSpecialization, isInstantiated);
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