// Copyright (c) 2017-2019 Florian Wende (flwende@gmail.com)
//
// Distributed under the BSD 2-clause Software License
// (See accompanying file LICENSE)

#if !defined(TRAFO_DATA_LAYOUT_PROXY_GEN_HPP)
#define TRAFO_DATA_LAYOUT_PROXY_GEN_HPP

#include <cstdint>
#include <iostream>
#include <memory>
#include <set>
#include <vector>
#include <fstream>
#include <algorithm>

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/PrettyPrinter.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/Support/raw_ostream.h>

#include <misc/ast_helper.hpp>
#include <misc/matcher.hpp>
#include <misc/rewriter.hpp>
#include <misc/string_helper.hpp>
#include <trafo/data_layout/class_meta_data.hpp>
#include <trafo/data_layout/variable_declaration.hpp>

#if !defined(TRAFO_NAMESPACE)
    #define TRAFO_NAMESPACE fw
#endif

namespace TRAFO_NAMESPACE
{
    using namespace TRAFO_NAMESPACE::internal;

    class InsertProxyClassImplementation : public clang::ASTConsumer
    {
        Rewriter rewriter;
        static std::shared_ptr<clang::Preprocessor> preprocessor;
        
        std::vector<const Declaration*> declarations;
        std::set<std::string> proxyClassTargetNames;
        std::vector<std::unique_ptr<ClassMetaData>> proxyClassTargets;
        const std::string proxyNamespace = std::string("proxy_internal");
        
        bool isThisClassInstantiated(const clang::CXXRecordDecl* const decl)
        {
            using namespace clang::ast_matchers;

            if (!decl) return false;

            Matcher matcher;
            bool testResult = false;

            matcher.addMatcher(cxxRecordDecl(allOf(hasName(decl->getNameAsString()), isInstantiated())).bind("test"),
                [&decl, &testResult] (const MatchFinder::MatchResult& result) mutable
                {
                    if (const clang::CXXRecordDecl* const instance = result.Nodes.getNodeAs<clang::CXXRecordDecl>("test"))
                    {
                        if (instance->getSourceRange() == decl->getSourceRange())
                        {
                            testResult = true;
                        }
                    }
                });

            matcher.run(decl->getASTContext());
                            
            return testResult;                
        }

        bool matchDeclarations(const std::vector<std::string>& containerNames, clang::ASTContext& context)
        {
            using namespace clang::ast_matchers;

            Matcher matcher;

            declarations.clear();

            for (auto containerName : containerNames)
            {
                matcher.addMatcher(varDecl(hasType(cxxRecordDecl(hasName(containerName)))).bind("varDecl"),
                    [&containerNames, &context, this] (const MatchFinder::MatchResult& result) mutable
                    {
                        if (const clang::VarDecl* const decl = result.Nodes.getNodeAs<clang::VarDecl>("varDecl"))
                        {
                            ContainerDeclaration containerDecl = ContainerDeclaration::make(*decl, context, containerNames);
                            const clang::Type* const type = containerDecl.elementDataType.getTypePtrOrNull();
                            const bool isRecordType = (type ? type->isRecordType() : false);

                            if (!containerDecl.elementDataType.isNull() && isRecordType)
                            {
                                declarations.push_back(new ContainerDeclaration(containerDecl));
                                proxyClassTargetNames.insert(containerDecl.elementDataTypeName);
                            }
                        }
                    });
            }

            matcher.addMatcher(varDecl(hasType(constantArrayType())).bind("constArrayDecl"),
                [&context, this] (const MatchFinder::MatchResult& result) mutable
                {
                    if (const clang::VarDecl* const decl = result.Nodes.getNodeAs<clang::VarDecl>("constArrayDecl"))
                    {
                        ConstantArrayDeclaration arrayDecl = ConstantArrayDeclaration::make(*decl, context);
                        const clang::Type* const type = arrayDecl.elementDataType.getTypePtrOrNull();
                        const bool isRecordType = (type ? type->isRecordType() : false);

                        if (!arrayDecl.elementDataType.isNull() && isRecordType)
                        {
                            declarations.push_back(new ConstantArrayDeclaration(arrayDecl));
                            proxyClassTargetNames.insert(arrayDecl.elementDataTypeName);
                        }
                    }
                });

            matcher.run(context);

            return (declarations.size() > 0);
        }

        bool findProxyClassTargets(clang::ASTContext& context)
        {
            using namespace clang::ast_matchers;

            Matcher matcher;

            for (auto name : proxyClassTargetNames)
            {
                // template class declarations
                matcher.addMatcher(classTemplateDecl(hasName(name)).bind("classTemplateDeclaration"),
                    [this, &context] (const MatchFinder::MatchResult& result) mutable
                    {
                        if (const clang::ClassTemplateDecl* const decl = result.Nodes.getNodeAs<clang::ClassTemplateDecl>("classTemplateDeclaration"))
                        {
                            if (decl->isThisDeclarationADefinition()) return;

                            const std::string sourceLocationString = decl->getBeginLoc().printToString(context.getSourceManager());
                            
                            if (sourceLocationString.find("usr/lib") != std::string::npos ||
                                sourceLocationString.find("usr/include") != std::string::npos) return;

                            proxyClassTargets.emplace_back(new TemplateClassMetaData(*decl, false));
                        }
                    });
                
                // template class definitions
                matcher.addMatcher(classTemplateDecl(hasName(name)).bind("classTemplateDefinition"),
                    [this, &context] (const MatchFinder::MatchResult& result) mutable
                    {
                        if (const clang::ClassTemplateDecl* const decl = result.Nodes.getNodeAs<clang::ClassTemplateDecl>("classTemplateDefinition"))
                        {
                            if (!decl->isThisDeclarationADefinition()) return;

                            const std::string sourceLocationString = decl->getBeginLoc().printToString(context.getSourceManager());
                            
                            if (sourceLocationString.find("usr/lib") != std::string::npos ||
                                sourceLocationString.find("usr/include") != std::string::npos) return;
                            
                            for (auto& target : proxyClassTargets)
                            {
                                if (target->addDefinition(*(decl->getTemplatedDecl()))) return;
                            }
                                       
                            // not found
                            proxyClassTargets.emplace_back(new TemplateClassMetaData(*decl, true));
                            proxyClassTargets.back()->addDefinition(*(decl->getTemplatedDecl()));
                        }
                    });
                
                // template class partial specialization
                matcher.addMatcher(classTemplatePartialSpecializationDecl(hasName(name)).bind("classTemplatePartialSpecialization"),
                    [this, &context] (const MatchFinder::MatchResult& result) mutable
                    {
                        if (const clang::ClassTemplatePartialSpecializationDecl* const decl = result.Nodes.getNodeAs<clang::ClassTemplatePartialSpecializationDecl>("classTemplatePartialSpecialization"))
                        {                         
                            const std::string sourceLocationString = decl->getBeginLoc().printToString(context.getSourceManager());
                            
                            if (sourceLocationString.find("usr/lib") != std::string::npos ||
                                sourceLocationString.find("usr/include") != std::string::npos) return;

                            const bool isInstantiated = isThisClassInstantiated(decl);
                            
                            for (std::size_t i = 0; i < proxyClassTargets.size(); ++i)
                            {
                                if (proxyClassTargets[i]->addDefinition(*decl, true, isInstantiated)) return;
                            }
                        }
                    });
                    
                // standard C++ classes
                matcher.addMatcher(cxxRecordDecl(allOf(hasName(name), unless(isTemplateInstantiation()))).bind("c++Class"),
                    [this, &context] (const MatchFinder::MatchResult& result) mutable
                    {
                        if (const clang::CXXRecordDecl* const decl = result.Nodes.getNodeAs<clang::CXXRecordDecl>("c++Class"))
                        {
                            const std::string name = decl->getNameAsString();
                            const bool isDefinition = decl->isThisDeclarationADefinition();

                            const std::string sourceLocationString = decl->getBeginLoc().printToString(context.getSourceManager());
                            
                            if (sourceLocationString.find("usr/lib") != std::string::npos ||
                                sourceLocationString.find("usr/include") != std::string::npos) return;

                            for (auto& target : proxyClassTargets)
                            {
                                // if it is a specialization of a template class, skip this class definition!
                                // note: if there are templated classes with that name, they (should) have been found by any previous matcher
                                if (target->name == name)
                                {
                                    if (target->isTemplated()) return;

                                    if (isDefinition && target->addDefinition(*decl, false)) return;
                                }
                            }

                            proxyClassTargets.emplace_back(new CXXClassMetaData(*decl, isDefinition));
                            if (isDefinition)
                            {
                                proxyClassTargets.back()->addDefinition(*decl, false);
                            }
                        }
                    });
            }

            matcher.run(context);

            return (proxyClassTargets.size() > 0);
        }
        
        std::string generateProxyClassDeclaration(const ClassMetaData::Declaration& declaration)
        {
            const std::string indent(declaration.indent.value, ' ');
            const std::string extIndent(declaration.indent.value + declaration.indent.increment, ' ');
            std::stringstream proxyClassDeclaration;

            proxyClassDeclaration << indent << "namespace " << proxyNamespace << "\n";
            proxyClassDeclaration << indent << "{\n";
            if (declaration.templateParameters.size())
            {
                const clang::SourceManager& sourceManager = declaration.getSourceManager();
                const std::string parameterList = dumpSourceRangeToString(declaration.templateParameterListSourceRange, sourceManager);

                proxyClassDeclaration << extIndent << parameterList << ">\n";
            }
            proxyClassDeclaration << extIndent << (declaration.isStruct ? "struct " : "class ") << declaration.name << "_proxy;\n";
            proxyClassDeclaration << indent << "}\n\n";

            return proxyClassDeclaration.str();
        }

        std::string generateProxyClassUsingStmt(const ClassMetaData::Definition& definition)
        {
            const Indentation insideClassIndent = definition.declaration.indent + 1;
            const std::string indent(insideClassIndent.value, ' ');
            std::stringstream usingStmt;

            usingStmt << indent << "using type = " << definition.name << definition.templateParameterString << ";\n\n";
            usingStmt << indent << "using proxy_type = typename " << definition.declaration.namespaceString << proxyNamespace << "::" << definition.name << "_proxy";
            usingStmt << definition.templateParameterString << ";\n";

            return usingStmt.str();
        }

        std::string generateConstructorInitializerList(const ClassMetaData::Definition& definition, const std::string rhs = std::string("rhs"), const Indentation indentation = Indentation(0))
        {
            const std::string indent(indentation.value, ' ');

            if (const std::uint32_t numInitializers = definition.fields.size())
            {
                std::stringstream initializerList;

                initializerList << indent << ":\n";
                for (std::uint32_t i = 0; i < numInitializers; ++i)
                {
                    const std::string fieldName = definition.fields[i].name;
                    initializerList << indent << fieldName << "(" << rhs << "." << fieldName;
                    initializerList << ((i + 1) < numInitializers ? "),\n" : ")\n");
                }

                return initializerList.str();
            }
            
            return std::string("");
        }

        std::string generateConstructorClassFromProxyClass(const ClassMetaData::Definition& definition)
        {
            const Indentation insideClassIndent = definition.declaration.indent + 1;
            const std::string indent(insideClassIndent.value, ' ');
            std::stringstream constructorDefinition;

            constructorDefinition << indent;

            if (definition.hasCopyConstructor)
            {
                const auto& constructor = definition.constructors[definition.indexCopyConstructor];
                const clang::ParmVarDecl* const parameter = constructor.decl.getParamDecl(0); // never nullptr

                if (constructor.isTemplatedConstructor)
                {
                    constructorDefinition << dumpSourceRangeToString(constructor.templateSignatureSourceRange, definition.getSourceManager());
                }
                constructorDefinition << definition.name << "(const " << proxyNamespace << "::" << definition.name << "_proxy";
                if (constructor.isTemplatedConstructor)
                {
                    const internal::ClassMetaData::FunctionArgument argument(*parameter, &definition);
                    constructorDefinition << argument.templateParameters;
                }
                constructorDefinition << "& ";
                
                // dump the definition of the copy constructor starting at the parameter name
                const clang::SourceRange everythingFromParmVarDeclName(parameter->getEndLoc(), constructor.decl.getEndLoc());
                constructorDefinition << dumpSourceRangeToString(everythingFromParmVarDeclName, definition.getSourceManager());
                if (constructor.decl.hasBody())
                {
                    constructorDefinition << "}";
                }
            }
            else
            {
                constructorDefinition << definition.name << "(const " << proxyNamespace << "::" << definition.name << "_proxy& other)\n";
                constructorDefinition << generateConstructorInitializerList(definition, std::string("other"), insideClassIndent + 1);
                constructorDefinition << indent << "{ ; }";
            }
            constructorDefinition << "\n";

            return constructorDefinition.str();
        }

        void modifyMethodSignature(const ClassMetaData::Definition& definition, const ClassMetaData::Function& method, Rewriter& rewriter, const std::string proxyNamespace, const std::string targetNamespace, const bool returnProxyType = false)
        {
            clang::ASTContext& context = definition.declaration.getASTContext();
            const clang::SourceManager& sourceManager = definition.declaration.getSourceManager();
            const clang::FileID fileId = definition.declaration.fileId;
            const Indentation insideClassIndent = definition.declaration.indent + 1;
            const std::string indent(insideClassIndent.value, ' ');
            const std::string extIndent((insideClassIndent + 1).value, ' ');
            const std::string proxyTypeName = proxyNamespace + method.className + std::string("_proxy");
            const std::string targetTypeName = targetNamespace + method.className;

            if (method.isMacroExpansion)
            {
                std::string macroString = dumpSourceRangeToString(getExpansionSourceRange(method.sourceRange, context), sourceManager);                            
                std::string proxyMacroString = macroString;
                if (findAndReplace(macroString, method.className, targetTypeName, true, true))
                {
                    rewriter.replace(getExpansionSourceRange(method.sourceRange, context), macroString);
                }
                if (findAndReplace(proxyMacroString, method.className, proxyTypeName, true, true))
                {
                    rewriter.insert(getNextLine(getExpansionSourceLocation(method.sourceRange.getEnd(), context), context), indent + proxyMacroString + std::string("\n"));
                }
                
                // adapt return type
                if (returnProxyType && method.returnsClassTypeReference)
                {
                    const clang::SourceLocation beginLoc = getSpellingSourceLocation(method.returnTypeSourceRange.getBegin(), context);
                    const clang::SourceLocation endLoc = getSpellingSourceLocation(method.bodySourceRange.getBegin(), context).getLocWithOffset(-1);
                    std::string selection = dumpSourceRangeToString({beginLoc, endLoc}, sourceManager);
                    findAndReplace(selection, method.className, proxyTypeName, true, true);

                    rewriter.replace({beginLoc, endLoc}, selection);
                }
            }
            else
            {
                // duplicate the original function definition and insert it only if we have to change argument types in the orignal definition
                // NOTE 1: this must not be the case if the class-type arguments have default values!
                const std::string originalFunctionDefinition = dumpSourceRangeToString(method.extendedSourceRange, sourceManager);
                bool insertOriginalDefinition = false;

                // create new function
                std::stringstream functionSignature;
                bool insertNewDefinition = false;

                if (method.returnsClassType && returnProxyType)
                {
                    std::string tmp = method.signatureString[0];
                    findAndReplace(tmp, method.className, proxyTypeName, true, true);
                    functionSignature << tmp << "(";
                }
                else
                {
                    functionSignature << method.signatureString[0] << "(";
                }

                for (std::uint32_t i = 0; i < method.arguments.size(); ++i)
                {
                    const auto& argument = method.arguments[i];

                    if (argument.isClassType)
                    {
                        std::string targetArgumentTypeName = argument.elementTypeName;
                        std::string proxyArgumentTypeName = targetArgumentTypeName;
                        findAndReplace(targetArgumentTypeName, method.className, targetTypeName, true, true);
                        findAndReplace(proxyArgumentTypeName, method.className, proxyTypeName, true, true);
                        
                        std::string argumentStringPostfix = (method.isFunctionTemplate ? argument.templateParameters : std::string("")) +
                            std::string(argument.isRReference ? "&&" : "") + 
                            std::string(argument.isLReference ? "&" : "") + 
                            std::string(argument.isPointer ? "* " : " ") + 
                            argument.name;
                        std::string targetArgumentString = (argument.isConst ? "const " : "") + targetArgumentTypeName + argumentStringPostfix;
                        std::string proxyArgumentString = (argument.isConst ? "const " : "") + proxyArgumentTypeName + argumentStringPostfix;

                        if (argument.decl.hasDefaultArg())
                        {
                            std::string originalArgumentString = dumpSourceRangeToString(method.signature[i + 1], sourceManager);

                            rewriter.replace(argument.sourceRange, targetArgumentString + std::string(" ") + originalArgumentString.substr(originalArgumentString.rfind('=')));
                            insertNewDefinition = true;
                        }
                        else
                        {
                            // 1. replace argument by proxy argument in original function
                            rewriter.replace(argument.sourceRange, proxyArgumentString);
                            insertOriginalDefinition = true;
                        }

                        // 2. create proxy argument in new function
                        functionSignature << (i > 0 ? ", " : "") << proxyArgumentString;
                    }
                    else
                    {
                        functionSignature << (i > 0 ? ", " : "") << dumpSourceRangeToString(method.signature[i + 1], sourceManager);
                    }

                    if ((i + 1) ==  method.arguments.size())
                    {
                        functionSignature << (method.isConst ? ") const\n" : ")\n");
                    }
                }
                
                if (insertNewDefinition)
                {
                    rewriter.insert(getNextLine(method.sourceRange.getEnd(), context), std::string("\n") + indent + functionSignature.str());
                    rewriter.insert(getNextLine(method.sourceRange.getEnd(), context), indent + dumpSourceRangeToString(method.extendedBodySourceRange, sourceManager));
                }

                if (insertOriginalDefinition)
                {
                    if (returnProxyType)
                    {
                        if (method.returnsClassTypeReference)
                        {
                            std::string tmp = dumpSourceRangeToString(method.signatureSourceRange, sourceManager);
                            findAndReplace(tmp, method.className, proxyTypeName, false, true);
                            rewriter.insert(getNextLine(method.sourceRange.getEnd(), context), std::string("\n") + indent + tmp);
                            rewriter.insert(getNextLine(method.sourceRange.getEnd(), context), dumpSourceRangeToString(method.extendedBodySourceRange, sourceManager));
                        }
                    }
                    else
                    {
                        rewriter.insert(getNextLine(method.sourceRange.getEnd(), context), std::string("\n") + indent + originalFunctionDefinition);
                    }
                }
            }
        }


        void modifyOriginalSourceCode(const std::unique_ptr<ClassMetaData>& candidate, Rewriter& rewriter)
        {
            if (!candidate.get()) return;

            const ClassMetaData::Declaration& declaration = candidate->getDeclaration();
            clang::ASTContext& context = declaration.getASTContext();
            const clang::SourceManager& sourceManager = declaration.getSourceManager();
            const clang::FileID fileId = declaration.fileId;

            // insert proxy class forward declaration
            rewriter.insert(getBeginOfLine(declaration.sourceRange.getBegin(), context), generateProxyClassDeclaration(declaration));
            
            // add constructors with proxy class arguments
            const std::vector<ClassMetaData::Definition>& definitions = candidate->getDefinitions();
            for (const auto& definition : definitions)
            {
                const Indentation insideClassIndent = definition.declaration.indent + 1;
                const std::string indent(insideClassIndent.value, ' ');
                const std::string extIndent((insideClassIndent + 1).value, ' ');

                // in case of a C++ class, there must be at least one public access specifier
                // as we require proxy class candidates have at least one public field!
                clang::SourceLocation sourceLocation;

                if (definition.indexPublicAccessSpecifiers.size() > 0)
                {
                    sourceLocation = definition.accessSpecifiers[definition.indexPublicAccessSpecifiers[0]].scopeBegin;

                }
                else if (definition.declaration.isStruct)
                {
                    sourceLocation = definition.innerSourceRange.getBegin();
                }

                // insert using statement into class definition
                rewriter.insert(sourceLocation, generateProxyClassUsingStmt(definition));
                
                // insert constructor
                if (definition.hasCopyConstructor)
                {
                    sourceLocation = getNextLine(definition.constructors[definition.indexCopyConstructor].sourceRange.getEnd(), context);
                }
                else if (definition.indexPublicConstructors.size() > 0)
                {
                    sourceLocation = getNextLine(definition.constructors[definition.indexPublicConstructors.back()].sourceRange.getEnd(), context);
                }
                rewriter.insert(sourceLocation, generateConstructorClassFromProxyClass(definition));

                // insert methods that have the class type as argument type
                for (const auto& method : definition.cxxMethods)
                {
                    if (method.hasClassTypeArguments)
                    { 
                        modifyMethodSignature(definition, method, rewriter, proxyNamespace + std::string("::"), std::string(""));
                    }
                }
            }
            
            // insert include line outside the outermost namespace or after the last class definition if there is no namespace
            std::stringstream includeLine;
            includeLine << "\n#include \"autogen_" << candidate->name << "_proxy.hpp\"\n\n";
            rewriter.insert(getNextLine(candidate->bottomMostSourceLocation, context), includeLine.str());

            // insert type traits
            {
                const ClassMetaData::Declaration& declaration = candidate->getDeclaration();
                
                rewriter.insert(getNextLine(candidate->bottomMostSourceLocation, context), std::string("#include <common/traits.hpp>\n\n"));    
                
                const Indentation Indent = Indentation(declaration.indent.increment, declaration.indent.increment);
                const std::string indent(Indent.value, ' ');
                const Indentation ExtIndent = Indent + 1;
                const std::string extIndent(ExtIndent.value, ' ');
                const Indentation ExtExtIndent = ExtIndent + 1;
                const std::string extExtIndent(ExtExtIndent.value, ' ');
                std::string namespaceName;
                for (const auto& thisNamespace : declaration.namespaces)
                {
                    namespaceName += (thisNamespace.name + std::string("::"));
                }

                
                std::stringstream typeTraitsStream_1;
                typeTraitsStream_1 << "namespace XXX_NAMESPACE\n{\n";
                typeTraitsStream_1 << indent << "namespace internal\n";
                typeTraitsStream_1 << indent << "{\n";
                if (candidate->isTemplated())
                {
                    typeTraitsStream_1 << extIndent << "template " << declaration.templateParameterDeclString << "\n";
                }
                typeTraitsStream_1 << extIndent << "struct provides_proxy_type<";

                std::stringstream typeTraitsStream_2;
                typeTraitsStream_2 << extIndent << "{\n";
                typeTraitsStream_2 << extExtIndent << "static constexpr bool value = true;\n";
                typeTraitsStream_2 << extIndent << "};\n";
                typeTraitsStream_2 << indent << "}\n";
                typeTraitsStream_2 << "}\n";

                rewriter.insert(getNextLine(candidate->bottomMostSourceLocation, context), typeTraitsStream_1.str());
                rewriter.insert(getNextLine(candidate->bottomMostSourceLocation, context), namespaceName + declaration.name + declaration.templateParameterString + std::string(">\n"));
                rewriter.insert(getNextLine(candidate->bottomMostSourceLocation, context), typeTraitsStream_2.str() + std::string("\n"));

                rewriter.insert(getNextLine(candidate->bottomMostSourceLocation, context), typeTraitsStream_1.str());
                rewriter.insert(getNextLine(candidate->bottomMostSourceLocation, context), std::string("const ") + namespaceName + declaration.name + declaration.templateParameterString + std::string(">\n"));
                rewriter.insert(getNextLine(candidate->bottomMostSourceLocation, context), typeTraitsStream_2.str());
            }

        }

        std::string generateProxyClassConstructor(const ClassMetaData::Definition& definition)
        {
            std::stringstream constructor;
            const Indentation Indent = definition.declaration.indent + 1;
            const std::string indent(Indent.value, ' ');
            const Indentation ExtIndent = definition.declaration.indent + 2;
            const std::string extIndent(ExtIndent.value, ' ');

            constructor << definition.name << "_proxy(base_pointer base)\n" << extIndent << ":\n";

            if (definition.isHomogeneous)
            {
                std::uint32_t fieldId = 0;
                for (const auto& field : definition.fields)
                {
                    constructor << extIndent << field.name << "(base.ptr[" << fieldId << " * base.n_0])" << ((fieldId + 1) < definition.fields.size() ? ",\n" : "\n");
                    ++fieldId;
                }
            }
            else
            {
                std::uint32_t fieldId = 0;
                for (const auto& field : definition.fields)
                {
                    constructor << extIndent << field.name << "(*(std::get<" << fieldId << ">(base.ptr)))" << ((fieldId + 1) < definition.fields.size() ? ",\n" : "\n");
                    ++fieldId;
                }
            }
            constructor << indent << "{}\n";
            
            return constructor.str();
        }

        std::string generateProxyClassTupleConstructor(const ClassMetaData::Definition& definition)
        {
            std::stringstream constructor;
            const Indentation Indent = definition.declaration.indent + 1;
            const std::string indent(Indent.value, ' ');
            const Indentation ExtIndent = definition.declaration.indent + 2;
            const std::string extIndent(ExtIndent.value, ' ');

            constructor << definition.name << "_proxy(" << "std::tuple<";

            std::uint32_t fieldId = 0;
            for (const auto& field : definition.fields)
            {
                constructor << (fieldId == 0 ? "" : ", ") << field.elementTypeName << "&";
                ++fieldId;
            }
            constructor << "> obj)\n" << extIndent << ":\n";
            fieldId = 0;
            for (const auto& field : definition.fields)
            {
                constructor << extIndent << field.name << "(std::get<" << fieldId << ">(obj))" << ((fieldId + 1) < definition.fields.size() ? ",\n" : "\n");
                ++fieldId;
            }
            constructor << indent << "{}\n";

            return constructor.str();
        }

        void generateProxyClassDefinition(const std::unique_ptr<ClassMetaData>& candidate, Rewriter& rewriter, const std::string header = std::string(""))
        {
            if (!candidate.get()) return;

            // some file related locations
            const clang::SourceManager& sourceManager = candidate->sourceManager;
            clang::ASTContext& context = candidate->context;
            const clang::FileID fileId = candidate->getDeclaration().fileId;
            const clang::SourceLocation fileStart = sourceManager.getLocForStartOfFile(fileId);
            const clang::SourceLocation fileEnd = sourceManager.getLocForEndOfFile(fileId);

            // insert iterator class forward declaration (top level namespace) and include lines
            {
                const ClassMetaData::Declaration& declaration = candidate->getDeclaration();
                const clang::SourceLocation insertLocation = (declaration.namespaces.size() > 0 ? declaration.namespaces[0].sourceRange.getBegin() : declaration.sourceRange.getBegin());
                
                rewriter.insert(insertLocation, std::string("#include <common/memory.hpp>\n"));
                rewriter.insert(insertLocation, std::string("#include <common/data_layout.hpp>\n\n"));
                
                const Indentation Indent = Indentation(declaration.indent.increment, declaration.indent.increment);
                const std::string indent(Indent.value, ' ');
                const Indentation ExtIndent = Indent + 1;
                const std::string extIndent(ExtIndent.value, ' ');

                std::stringstream iteratorForwardDeclStream;
                iteratorForwardDeclStream << "namespace XXX_NAMESPACE\n{\n";
                iteratorForwardDeclStream << indent << "namespace internal \n";
                iteratorForwardDeclStream << indent << "{\n";
                iteratorForwardDeclStream << extIndent << "template <typename P, typename R>\n";
                iteratorForwardDeclStream << extIndent << "class iterator;\n";
                iteratorForwardDeclStream << indent << "}\n";
                iteratorForwardDeclStream << "}\n\n";
                rewriter.insert(insertLocation, iteratorForwardDeclStream.str());

                std::stringstream accessorForwardDeclStream;
                accessorForwardDeclStream << "namespace XXX_NAMESPACE\n{\n";
                accessorForwardDeclStream << indent << "namespace internal \n";
                accessorForwardDeclStream << indent << "{\n";
                accessorForwardDeclStream << extIndent << "template <typename X, std::size_t N, std::size_t D, XXX_NAMESPACE::data_layout L>\n";
                accessorForwardDeclStream << extIndent << "class accessor;\n";
                accessorForwardDeclStream << indent << "}\n";
                accessorForwardDeclStream << "}\n\n";
                rewriter.insert(insertLocation, accessorForwardDeclStream.str());
            }


            // declarations for which proxy equivalents need to be generated
            const std::uint32_t numRelevantSourceRanges = candidate->relevantSourceRanges.size(); // is at least 1
            ClassMetaData::SourceRangeSet::const_iterator sourceRange = candidate->relevantSourceRanges.cbegin();

            // replace header
            const clang::SourceRange fileStartToFirstDeclBegin(fileStart, sourceRange->getBegin().getLocWithOffset(-1));
            if (fileStartToFirstDeclBegin.isValid())
            { 
                rewriter.replace(fileStartToFirstDeclBegin, header);
            }

            // remove everything that is not relevant for the proxy generation
            for (std::uint32_t i = 0; i < (numRelevantSourceRanges - 1); ++i)
            {

                const clang::SourceLocation start = sourceRange->getEnd().getLocWithOffset(1);
                const clang::SourceLocation end = (++sourceRange)->getBegin().getLocWithOffset(-1);
                const clang::SourceRange toBeRemoved(start, end);
                if (toBeRemoved.isValid() && toBeRemoved.getBegin() != toBeRemoved.getEnd())
                {
                    rewriter.remove(toBeRemoved);
                }
            }

            const clang::SourceRange toBeRemoved(sourceRange->getEnd().getLocWithOffset(1), fileEnd);
            if (toBeRemoved.isValid() && toBeRemoved.getBegin() != toBeRemoved.getEnd())
            {
                rewriter.replace(toBeRemoved, std::string("\n"));
            }

            // declaration: replace class name by proxy class name
            const std::string proxyNamespace = std::string("proxy_internal");

            const ClassMetaData::Declaration& declaration = candidate->getDeclaration();
            if (!declaration.isDefinition)
            {
                // insert proxy namespace
                const Indentation Indent = declaration.indent;
                const std::string indent(Indent.value, ' ');

                rewriter.insert(declaration.extendedSourceRange.getBegin(), indent + std::string("namespace ") + proxyNamespace + std::string("\n") + indent + std::string("{\n"));
                rewriter.insert(declaration.extendedSourceRange.getEnd(), indent + std::string("\n") + indent + std::string("}\n"));

                rewriter.replace(declaration.nameSourceRange, declaration.name + std::string("_proxy"));
            }

            // target class namespace prefix
            std::string targetClassNamespace;
            for (const auto& thisNamespace : candidate->getDeclaration().namespaces)
            {
                targetClassNamespace += (thisNamespace.name + std::string("::"));
            }

            // definitions
            const std::vector<ClassMetaData::Definition>& definitions = candidate->getDefinitions();
            for (const auto& definition : definitions)
            {
                // insert proxy namespace
                const Indentation Indent = definition.declaration.indent;
                const std::string indent(Indent.value, ' ');

                rewriter.insert(definition.extendedSourceRange.getBegin(), indent + std::string("namespace ") + proxyNamespace + std::string("\n") + indent + std::string("{\n"));
                rewriter.insert(definition.extendedSourceRange.getEnd(), indent + std::string("\n") + indent + std::string("}\n"));

                // replace class name by proxy class name
                rewriter.replace(definition.nameSourceRange, definition.name + std::string("_proxy"));

                // indentation
                const Indentation ExtIndent = Indent + 1;
                const std::string extIndent(ExtIndent.value, ' ');
                const Indentation ExtExtIndent = ExtIndent + 1;
                const std::string extExtIndent(ExtExtIndent.value, ' ');

                // open private region
                rewriter.insert(definition.innerLocBegin, std::string("\n") + indent + std::string("private:\n"));

                // friend declarations
                rewriter.insert(definition.innerLocBegin, std::string("\n") + extIndent + std::string("template <typename _X, std::size_t _N, std::size_t _D, XXX_NAMESPACE::data_layout _L>\n") + extIndent + std::string("friend class XXX_NAMESPACE::internal::accessor;\n"));
                rewriter.insert(definition.innerLocBegin, std::string("\n") + extIndent + std::string("template <typename _P, std::size_t _R>\n") + extIndent + std::string("friend class XXX_NAMESPACE::internal::iterator;\n"));

                // insert meta data: unqualified type
                for (const auto& templateParamter : definition.declaration.templateParameters)
                {
                    if (!templateParamter.isTypeParameter) continue;

                    const std::string parameterName = templateParamter.name;

                    rewriter.insert(definition.innerLocBegin, std::string("\n") + extIndent + std::string("using ") + parameterName + std::string("_unqualified = typename std::remove_cv<") + parameterName + std::string(">::type;\n"));
                }

                // insert meta data: is const type?
                if (definition.declaration.templateParameters.size() > 0)
                {
                    std::stringstream isConstTypeStream;
                    isConstTypeStream << "\n" << extIndent << "static constexpr bool is_const_type = (";
                    std::uint32_t templateParameterId = 0;
                    for (const auto& templateParamter : definition.declaration.templateParameters)
                    {
                        if (!templateParamter.isTypeParameter) continue;

                        const std::string parameterName = templateParamter.name;

                        isConstTypeStream << (templateParameterId == 0 ? "" : " || " ) << "std::is_const<" << parameterName << ">::value";
                        ++templateParameterId;
                    }
                    isConstTypeStream << ");\n";
                    rewriter.insert(definition.innerLocBegin, isConstTypeStream.str());
                }

                // open public region
                rewriter.insert(definition.innerLocBegin, std::string("\n") + indent + std::string("public:\n"));

                const clang::SourceLocation publicAccess = definition.accessSpecifiers[definition.indexPublicAccessSpecifiers[0]].scopeBegin;

                // insert meta data: type and const_type
                rewriter.insert(definition.innerLocBegin, std::string("\n") + extIndent + std::string("using type = ") + definition.name + std::string("_proxy") + definition.templateParameterString + std::string(";\n"));
                rewriter.insert(definition.innerLocBegin, std::string("\n") + extIndent + std::string("using const_type = ") + definition.name + std::string("_proxy") + definition.templateConstParameterString + std::string(";\n"));

                // insert meta data: base pointer type?
                std::stringstream basePointerStream;
                basePointerStream << "\n" << extIndent << "using base_pointer = XXX_NAMESPACE::multi_pointer_";
                if (definition.isHomogeneous)
                {
                    basePointerStream << "n<" << definition.fields[0].elementTypeName << ", " << std::to_string(definition.fields.size());
                }
                else
                {
                    basePointerStream << "inhomogeneous<";

                    std::uint32_t fieldId = 0;
                    for (const auto& field : definition.fields)
                    {
                        basePointerStream << (fieldId == 0 ? "" : ", ") << field.elementTypeName;
                        ++fieldId;
                    }
                }
                basePointerStream << ">;\n";
                rewriter.insert(definition.innerLocBegin, basePointerStream.str());

                // insert meta data: original type
                std::stringstream originalType;
                {
                    originalType << targetClassNamespace << definition.name;
                    std::uint32_t templateParameterId = 0;
                    for (const auto& templateParamter : definition.declaration.templateParameters)
                    {
                        const std::string parameterName = (definition.isTemplatePartialSpecialization ? definition.templatePartialSpecializationArguments[templateParameterId].first : templateParamter.name) +
                            std::string(templateParamter.isTypeParameter ? "_unqualified" : "");

                        originalType << (templateParameterId == 0 ? "<" : ", ") << parameterName;
                        ++templateParameterId;
                        if (templateParameterId == definition.declaration.templateParameters.size())
                        {
                            originalType << ">";        
                        }
                    }
                }

                std::stringstream originalTypeStream;
                if (definition.declaration.templateParameters.size() > 0)
                {
                    originalTypeStream << "\n" << extIndent << "using original_type = typename std::conditional<is_const_type,\n";
                    originalTypeStream << extExtIndent << "const " << originalType.str() << ",\n";
                    originalTypeStream << extExtIndent << originalType.str();
                    originalTypeStream << ">::type;\n";

                }
                else
                {
                    originalTypeStream << "\n" << extIndent << "using original_type = " << definition.name << ";\n";
                }
                
                rewriter.insert(definition.innerLocBegin, originalTypeStream.str());

                // open private region for classes
                if (definition.declaration.isClass)
                {
                    rewriter.insert(definition.innerLocBegin, std::string("\n") + indent + std::string("private:\n"));
                }

                // constructors
                std::vector<std::string> proxyClassConstructors;
                proxyClassConstructors.emplace_back(generateProxyClassConstructor(definition));
                proxyClassConstructors.emplace_back(generateProxyClassTupleConstructor(definition));

                const std::uint32_t numConstructorsToBeInserted = proxyClassConstructors.size();
                const std::uint32_t numConstructorLocationsAvailable = definition.indexPublicConstructors.size();
                const std::uint32_t numConstructorsInserted = std::min(numConstructorLocationsAvailable, numConstructorsToBeInserted);
                for (std::uint32_t i = 0; i < numConstructorsInserted; ++i)
                {
                    rewriter.replace(definition.constructors[definition.indexPublicConstructors[i]].sourceRange, proxyClassConstructors[i]);
                }
                const clang::SourceLocation fallbackSourceLocation = (definition.indexPublicAccessSpecifiers.size() > 0 ? definition.accessSpecifiers[definition.indexPublicAccessSpecifiers[0]].scopeBegin : definition.innerLocBegin);
                for (std::uint32_t i = numConstructorsInserted; i < proxyClassConstructors.size(); ++i)
                {
                    rewriter.insert(fallbackSourceLocation, std::string("\n") + proxyClassConstructors[i]);
                }

                for (std::uint32_t i = numConstructorsInserted; i < definition.constructors.size(); ++i)
                {
                    rewriter.remove(definition.constructors[i].sourceRange);
                }

                // adapt functions
                for (const auto& method : definition.cxxMethods)
                {
                    if (!method.hasClassTypeArguments && !method.returnsClassType) continue;

                    // only return value is of class type
                    if (method.returnsClassType && !method.hasClassTypeArguments)
                    {
                        if (method.returnsClassTypeReference)
                        {
                            rewriter.replace(getSpellingSourceRange(method.returnTypeSourceRange, context), definition.name + std::string("_proxy"));
                        }
                        else
                        {
                            // TODO: fix that -> for some reason the closing bracket of the body is not included -> add "1"
                            const clang::SourceRange selectionSourceRange(method.macroDefinitionSourceRange.getBegin(), method.macroDefinitionSourceRange.getEnd().getLocWithOffset(1));
                            std::string selection = dumpSourceRangeToString(selectionSourceRange, sourceManager);
                            findAndReplace(selection, definition.name, originalType.str(), true, true);
                            rewriter.replace(selectionSourceRange, selection);
                        }
                    }

                    // return value and arguments can be of class type
                    if (method.hasClassTypeArguments)
                    {
                        modifyMethodSignature(definition, method, rewriter, std::string(""), targetClassNamespace, true);
                    }
                }

                // change field types -> reference values
                for (const auto& field : definition.fields)
                {
                    rewriter.replace(field.sourceRange, field.typeName + std::string("& ") + field.name);
                }
            }
        }

        std::vector<const clang::FunctionDecl*> findFunctionsWithTargetParameters(const ClassMetaData& target)
        {
            using namespace clang::ast_matchers;

            Matcher matcher;
            clang::ASTContext& context = target.context;
            std::vector<const clang::FunctionDecl*> functions;
            
            matcher.addMatcher(functionDecl().bind("functionDeclaration"),
                    [this, &target, &context, &functions] (const MatchFinder::MatchResult& result) mutable
                    {
                        if (const clang::FunctionDecl* const decl = result.Nodes.getNodeAs<clang::FunctionDecl>("functionDeclaration"))
                        {
                            const std::string sourceLocationString = decl->getBeginLoc().printToString(context.getSourceManager());
                            
                            if (sourceLocationString.find("usr/lib") != std::string::npos ||
                                sourceLocationString.find("usr/include") != std::string::npos) return;

                            if (decl->getNumParams() == 0) return;

                            bool hasTargetArgument = false;
                            for (const auto& parameter : decl->parameters())
                            {
                                if (parameter->getType().getAsString().find(target.getDeclaration().name) != std::string::npos)
                                {
                                    hasTargetArgument = true;
                                    break;
                                }
                            }

                            if (!hasTargetArgument) return;

                            // skip all functions that are within the definition of any of the classes considered
                            for (const auto& definition : target.getDefinitions())
                            {
                                const std::size_t defBeginLine = getSpellingLineNumber(definition.sourceRange.getBegin(), context);
                                const std::size_t defEndLine = getSpellingLineNumber(definition.sourceRange.getEnd(), context);
                                const std::size_t declBeginLine = getSpellingLineNumber(decl->getSourceRange().getBegin(), context);

                                if (defBeginLine < declBeginLine && declBeginLine < defEndLine) return;
                            }

                            for (const auto& definitionSourceRange : target.getDefinitionsSortedOut())
                            {
                                const std::size_t defBeginLine = getSpellingLineNumber(definitionSourceRange.getBegin(), context);
                                const std::size_t defEndLine = getSpellingLineNumber(definitionSourceRange.getEnd(), context);
                                const std::size_t declBeginLine = getSpellingLineNumber(decl->getSourceRange().getBegin(), context);

                                if (defBeginLine < declBeginLine && declBeginLine < defEndLine) return;
                            }

                            const auto& it = std::lower_bound(functions.begin(), functions.end(), decl, [&context] (const clang::FunctionDecl* d_1, const clang::FunctionDecl* d_2)
                                {
                                    const std::size_t beginLoc_d_1 = getSpellingLineNumber(d_1->getSourceRange().getBegin(), context);
                                    const std::size_t beginLoc_d_2 = getSpellingLineNumber(d_2->getSourceRange().getBegin(), context);
                                    return (beginLoc_d_1 < beginLoc_d_2);
                                });


                            if (it == functions.end() || (*it)->getSourceRange() != decl->getSourceRange())
                            {
                                functions.insert(it, decl);
                            }
                        }
                    });

            matcher.run(target.context);

            return functions;
        }
        
        void addProxyClassToSource(const std::vector<std::unique_ptr<ClassMetaData>>& proxyClassTargets, clang::ASTContext& context)
        {
            // make a hard copy before applying any changes to the source code
            Rewriter proxyClassCreator(rewriter);
        
            for (const auto& target : proxyClassTargets)
            {
                if (!target->containsProxyClassCandidates) continue;

                // check if the source file containing the original code, also contains function definitions with class type paramters: add them if needed
                std::vector<const clang::FunctionDecl*> functionsWithTargetParameters = findFunctionsWithTargetParameters(*target);

                for (const auto& function : functionsWithTargetParameters)
                {
                    const clang::FileID fileId = context.getSourceManager().getFileID(function->getLocation());
                    if (fileId == target->fileId)
                    {
                        const clang::FunctionTemplateDecl* templateFunctionDecl = function->getDescribedFunctionTemplate();
                        const clang::SourceRange functionSourceRange = (templateFunctionDecl == nullptr ? function->getSourceRange() : templateFunctionDecl->getSourceRange());
                        target->getRelevantSourceRanges().insert(clang::SourceRange(getBeginOfLine(functionSourceRange.getBegin(), context), functionSourceRange.getEnd()));
                    }
                }
                target->updateRelevantSourceRangeInfo();

                //////////////////////////////////////////////////////////////////////////////////////////////////////////
                //////////////////////////////////////////////////////////////////////////////////////////////////////////

                // modify original source code
                modifyOriginalSourceCode(target, rewriter);

                {
                    std::string outputString;
                    llvm::raw_string_ostream outputStream(outputString);
                    rewriter.getEditBuffer(target->fileId).write(outputStream);
            
                    std::string outputFilename(target->filename);
                    const std::size_t pos = outputFilename.rfind('/');

                    if (const char* output_path = secure_getenv("CODE_TRAFO_OUTPUT_PATH"))
                    {
                        const std::string filename = outputFilename.substr(pos);
                        outputFilename = std::string(output_path) + filename;
                    }
                    else
                    {
                        outputFilename.insert(pos != std::string::npos ? (pos + 1) : 0, "new_files/");
                    }

                    std::ofstream out(outputFilename);
                    if (out)
                    {
                        out << outputStream.str();
                        out.close();
                    }
                    else
                    {
                        std::cerr << "error: unable to open file " << outputFilename << std::endl << std::flush;
                    }    
                }

                //////////////////////////////////////////////////////////////////////////////////////////////////////////
                //////////////////////////////////////////////////////////////////////////////////////////////////////////

                // generate proxy class definition
                generateProxyClassDefinition(target, proxyClassCreator, std::string("// my header\n"));

                // process functions that are not within the class
                for (auto it = functionsWithTargetParameters.begin(); it != functionsWithTargetParameters.end(); )
                {
                    const auto& function = *it;
                    const clang::FileID fileId = context.getSourceManager().getFileID(function->getLocation());
                    if (fileId == target->fileId)
                    {
                        const clang::FunctionTemplateDecl* templateFunctionDecl = function->getDescribedFunctionTemplate();
                        const clang::SourceRange functionSourceRange = (templateFunctionDecl == nullptr ? function->getSourceRange() : templateFunctionDecl->getSourceRange());
                        const std::string indent(getSpellingColumnNumber(functionSourceRange.getBegin(), context) - 1, ' ');
                        std::string functionDefinition = dumpSourceRangeToString({functionSourceRange.getBegin(), functionSourceRange.getEnd().getLocWithOffset(1)}, context.getSourceManager());

                        findAndReplace(functionDefinition, target->name, target->name + std::string("_proxy"), true, true);
                        proxyClassCreator.replace(functionSourceRange, functionDefinition);

                        const bool isInputOutputOperator = (function->getNameAsString() == std::string("operator<<") || function->getNameAsString() == std::string("operator>>"));
                        if (isInputOutputOperator)
                        {
                            proxyClassCreator.insert(functionSourceRange.getBegin(), std::string("namespace ") + proxyNamespace + std::string("\n") + indent + std::string("{\n") + indent);
                            proxyClassCreator.insert(functionSourceRange.getEnd().getLocWithOffset(1), std::string("\n") + indent + std::string("}"));
                        }

                       it = functionsWithTargetParameters.erase(it);
                       continue;
                    }

                    ++it;
                }


                if (functionsWithTargetParameters.size() > 0)
                {
                    proxyClassCreator.insert(target->bottomMostSourceLocation.getLocWithOffset(1), std::string("\n\n#include \"") + std::string("autogen_") + target->name + std::string("_proxy_func.hpp\""));
                }

                {
                    std::string outputString;
                    llvm::raw_string_ostream outputStream(outputString);
                    
                    proxyClassCreator.getEditBuffer(target->fileId).write(outputStream);

                    std::string outputFilename(target->filename);

                    if (const char* output_path = secure_getenv("CODE_TRAFO_OUTPUT_PATH"))
                    {
                        outputFilename = std::string(output_path);
                    }
                    else
                    {
                        const std::size_t pos = outputFilename.rfind('/');
                        outputFilename = outputFilename.substr(0, pos) + std::string("/new_files");
                    }
                    outputFilename += (std::string("/autogen_") + target->name + std::string("_proxy.hpp"));
                    
                    std::ofstream out(outputFilename);
                    if (out)
                    {
                        out << outputStream.str();
                        out.close();
                    }
                    else
                    {
                        std::cerr << "error: unable to open file " << outputFilename << std::endl << std::flush;
                    } 
                }

                //////////////////////////////////////////////////////////////////////////////////////////////////////////
                //////////////////////////////////////////////////////////////////////////////////////////////////////////

                // transform all remaining functions with target class parameters
                std::stringstream globalFunctionString;
                for (const auto& function : functionsWithTargetParameters)
                {
                    // deduce namespaces
                    std::string fn;
                    llvm::raw_string_ostream fullNameRawString(fn);
                    function->getNameForDiagnostic(fullNameRawString, clang::PrintingPolicy(clang::LangOptions()), true);
                    std::string fullName = fullNameRawString.str();
                    std::vector<std::string> namespaces;
                    std::size_t pos = fullName.find("::");

                    while (pos != std::string::npos)
                    {
                        namespaces.push_back(fullName.substr(0, pos));
                        fullName = fullName.substr(pos + 2);
                        pos = fullName.find("::");
                    }

                    // modify function signature
                    const clang::FunctionTemplateDecl* templateFunctionDecl = function->getDescribedFunctionTemplate();
                    const clang::SourceRange functionSourceRange = (templateFunctionDecl == nullptr ? function->getSourceRange() : templateFunctionDecl->getSourceRange());
                    const std::string indent(getSpellingColumnNumber(functionSourceRange.getBegin(), context) - 1, ' ');
                    const clang::SourceLocation parameterListBeginLoc = function->getParamDecl(0)->getSourceRange().getBegin();
                    const clang::SourceLocation parameterListEndLoc = (function->hasBody() ? function->getBody()->getSourceRange().getBegin() : functionSourceRange.getEnd());
                    std::string selection_1 = dumpSourceRangeToString({functionSourceRange.getBegin(), parameterListBeginLoc}, context.getSourceManager());
                    std::string selection_2 = dumpSourceRangeToString({parameterListBeginLoc, parameterListEndLoc}, context.getSourceManager());
                    std::string selection_3 = dumpSourceRangeToString({parameterListEndLoc, functionSourceRange.getEnd().getLocWithOffset(1)}, context.getSourceManager());

                    std::string proxyTypeNamespacePrefix;
                    for (const auto& ns : target->getDeclaration().namespaces)
                    {
                        proxyTypeNamespacePrefix += ns.name;
                    }
                    proxyTypeNamespacePrefix += std::string("::");
                    std::string proxyTypeName = proxyTypeNamespacePrefix + proxyNamespace + std::string("::") + target->name + std::string("_proxy");

                    if (function->getDeclaredReturnType()->isReferenceType())
                    {
                        findAndReplace(selection_1, proxyTypeNamespacePrefix + target->name, proxyTypeName, true, true);
                        findAndReplace(selection_1, target->name, proxyTypeName, true, true);
                    }
                    findAndReplace(selection_2, proxyTypeNamespacePrefix + target->name, proxyTypeName, true, true);
                    findAndReplace(selection_2, target->name, proxyTypeName, true, true);
                    const std::size_t numNamespaces = namespaces.size();

                    std::string extIndent;
                    for (std::size_t i = 0; i < numNamespaces; ++i)
                    {
                        globalFunctionString << extIndent << "namespace " << namespaces[i] << "\n";
                        globalFunctionString << extIndent << "{\n";

                        extIndent += indent;
                    }

                    globalFunctionString << extIndent << selection_1;
                    globalFunctionString << selection_2;
                    globalFunctionString << selection_3 << std::endl;

                    if (numNamespaces > 0)
                    {
                        for (std::size_t i = (numNamespaces - 1); i >= 0; --i)
                        {
                            extIndent = std::string("");
                            for (std::size_t j = 0; j < i; ++j)
                            {
                                extIndent += indent;
                            }
                            globalFunctionString << extIndent << "}\n\n";

                            if (i == 0) break;  
                        }
                    }
                }

                // write to file
                if (functionsWithTargetParameters.size() > 0)
                {
                    std::string outputFilename(target->filename);

                    if (const char* output_path = secure_getenv("CODE_TRAFO_OUTPUT_PATH"))
                    {
                        outputFilename = std::string(output_path);
                    }
                    else
                    {
                        const std::size_t pos = outputFilename.rfind('/');
                        outputFilename = outputFilename.substr(0, pos) + std::string("/new_files");
                    }
                    outputFilename += (std::string("/autogen_") + target->name + std::string("_proxy_func.hpp"));

                    std::ofstream out(outputFilename);
                    if (out)
                    {
                        out << globalFunctionString.str();
                        out.close();
                    }
                    else
                    {
                        std::cerr << "error: unable to open file " << outputFilename << std::endl << std::flush;
                    }
                }
            }
        }

        void modifyIncludeStatements(const std::vector<std::unique_ptr<ClassMetaData>>& proxyClassTargets, clang::ASTContext& context)
        {
            for (const auto& target : proxyClassTargets)
            {
                const clang::SourceLocation includeLocation = target->sourceManager.getIncludeLoc(target->fileId);
                const clang::SourceLocation includeLineBegin = getBeginOfLine(includeLocation, target->context);
                const clang::SourceLocation includeLineEnd = getNextLine(includeLocation, target->context).getLocWithOffset(-1);
                const std::size_t includeLineLength = getSpellingColumnNumber(includeLineEnd, target->context) - getSpellingColumnNumber(includeLineBegin, target->context);

                clang::RewriteBuffer& rewriteBuffer = rewriter.getEditBuffer(target->sourceManager.getFileID(includeLocation));
                rewriteBuffer.ReplaceText(target->sourceManager.getFileOffset(includeLineBegin), includeLineLength, std::string("#include \"") + target->name + std::string(".hpp\""));
                rewriteBuffer.InsertTextBefore(target->sourceManager.getFileOffset(includeLineBegin), std::string("#include <buffer/buffer.hpp>\n"));
            }
        }

        void modifyDeclarations(clang::ASTContext& context)
        {
            std::set<clang::FileID> outputFiles;
            const clang::SourceManager& sourceManager = context.getSourceManager();

            for (const auto& declaration : declarations)
            {
                declaration->printInfo(context.getSourceManager());

                const clang::SourceLocation declBegin = declaration->sourceRange.getBegin();
                const clang::SourceLocation declEnd = declaration->sourceRange.getEnd().getLocWithOffset(1);
                const std::size_t declLength = getSpellingColumnNumber(declEnd, context) - getSpellingColumnNumber(declBegin, context);

                const clang::FileID fileId = context.getSourceManager().getFileID(declBegin);
                clang::RewriteBuffer& rewriteBuffer = rewriter.getEditBuffer(fileId);
                outputFiles.insert(fileId);

                std::stringstream newDeclaration;
                newDeclaration << "XXX_NAMESPACE::buffer<" << declaration->elementDataType.getAsString() << ", ";
                newDeclaration << declaration->getNestingLevel() + 1 << ", ";
                newDeclaration << "XXX_NAMESPACE::data_layout::SoA> ";
                newDeclaration << declaration->decl.getNameAsString();

                bool nonZeroExtent = true;
                for (std::size_t i = 0; i <= declaration->getNestingLevel(); ++i)
                {
                    nonZeroExtent &= (declaration->getExtent().at(i) != 0);
                }

                if (nonZeroExtent)
                {
                    newDeclaration << "{{";
                    for (std::size_t i = 0; i <= declaration->getNestingLevel(); ++i)
                    {
                        newDeclaration << (i == 0 ? "" : ", ") << declaration->getExtentString().at(i);
                    }
                    newDeclaration << "}}";
                }
                newDeclaration << ";";

                rewriteBuffer.ReplaceText(context.getSourceManager().getFileOffset(declBegin), declLength, newDeclaration.str());
            }

            for (const clang::FileID fileId : outputFiles)
            {
                clang::RewriteBuffer& rewriteBuffer = rewriter.getEditBuffer(fileId);
                std::string outputString;
                llvm::raw_string_ostream outputStream(outputString);
                rewriteBuffer.write(outputStream);

                std::string outputFilename = sourceManager.getFilename(sourceManager.getLocForEndOfFile(fileId)).str();
                const std::size_t pos = outputFilename.rfind('/');

                if (const char* output_path = secure_getenv("CODE_TRAFO_OUTPUT_PATH"))
                {
                    const std::string filename = outputFilename.substr(pos);
                    outputFilename = std::string(output_path) + filename;
                }
                else
                {
                    outputFilename.insert(pos != std::string::npos ? (pos + 1) : 0, "new_files/");
                }
                
                std::ofstream out(outputFilename);
                if (out)
                {
                    out << outputStream.str();
                    out.close();
                }
                else
                {
                    std::cerr << "error: unable to open file " << outputFilename << std::endl << std::flush;
                }
            }
        }

    public:
        
        InsertProxyClassImplementation(clang::Rewriter& clangRewriter)
            :
            rewriter(clangRewriter)
        { ; }

        ~InsertProxyClassImplementation()
        {
            for (const auto& declaration : declarations)
            {
                if (declaration)
                {
                    delete declaration;
                }
            }
        }

        static void registerPreprocessor(std::shared_ptr<clang::Preprocessor> preprocessor)
        {
            if (!InsertProxyClassImplementation::preprocessor.get())
            {
                InsertProxyClassImplementation::preprocessor = preprocessor;
                ClassMetaData::registerPreprocessor(preprocessor);
            }
        }

        void HandleTranslationUnit(clang::ASTContext& context) override
        {	
            // step 1: find all relevant container declarations
            std::vector<std::string> containerNames;
            containerNames.push_back(std::string("vector"));
            containerNames.push_back(std::string("array"));
            if (!matchDeclarations(containerNames, context)) return;

            // step 2: check if element data type is candidate for proxy class generation
            if (!findProxyClassTargets(context)) return;

            // step 3: add proxy classes
            addProxyClassToSource(proxyClassTargets, context);

            // step 4: adapt includes
            modifyIncludeStatements(proxyClassTargets, context);

            // step 5: adapt declarations
            modifyDeclarations(context);
        }
    };

    std::shared_ptr<clang::Preprocessor> InsertProxyClassImplementation::preprocessor;

    class InsertProxyClass : public clang::ASTFrontendAction
    {
        clang::Rewriter rewriter;
        
    public:
        
        InsertProxyClass() { ; }
        
        void EndSourceFileAction() override
        {
            //rewriter.getEditBuffer(rewriter.getSourceMgr().getMainFileID()).write(llvm::outs());
        }
        
        std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance& compilerInstance, llvm::StringRef file) override
        {
            if (compilerInstance.hasPreprocessor())
            {
                InsertProxyClassImplementation::registerPreprocessor(compilerInstance.getPreprocessorPtr());
            }

            rewriter.setSourceMgr(compilerInstance.getSourceManager(), compilerInstance.getLangOpts());
            return llvm::make_unique<InsertProxyClassImplementation>(rewriter);
        }
    };
}

#endif