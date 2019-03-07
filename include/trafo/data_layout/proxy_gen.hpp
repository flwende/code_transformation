// Copyright (c) 2017-2018 Florian Wende (flwende@gmail.com)
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

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>

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
    /*
    class ClassDefinition : public clang::ast_matchers::MatchFinder::MatchCallback
    {
    protected:

        Rewriter& rewriter;
        std::vector<ClassMetaData> targetClasses;

        std::string createProxyClassStandardConstructor(ClassMetaData& thisClass, const clang::CXXRecordDecl& decl) const
        {
            std::string constructor = thisClass.name + std::string("_proxy(");

            if (thisClass.hasMultiplePublicFieldTypes)
            {
                std::cerr << thisClass.name << ": createProxyClassStandardConstructor: not implemented yet" << std::endl;
            }
            else
            {
                std::string publicFieldType;
                for (auto field : thisClass.fields)
                {
                    if (field.isPublic)
                    {
                        publicFieldType = field.typeName;
                        break;
                    }
                }

                constructor += publicFieldType + std::string("* ptr, const std::size_t n");
                if (thisClass.numFields > thisClass.numPublicFields)
                {
                    for (auto field : thisClass.fields)
                    {
                        if (!field.isPublic)
                        {
                            constructor +=  std::string(", ") + field.typeName + std::string(" ") + field.name;
                        }
                    }
                }
                constructor += ")\n\t:\n\t";

                std::uint32_t publicFieldId = 0;
                std::uint32_t fieldId = 0;
                for (auto field : thisClass.fields)
                {
                    if (field.isPublic)
                    {
                        constructor += field.name + std::string("(ptr[") + std::to_string(publicFieldId) + std::string(" * n])");
                        ++publicFieldId;
                    }
                    else
                    {
                        constructor += field.name + std::string("(") + field.name + std::string(")");
                    }

                    constructor += std::string((fieldId + 1) == thisClass.numFields ? "\n\t{ ; }" : ",\n\t");
                    ++fieldId;
                }
            }

            return constructor;
        }

        // generate proxy class definition
        void generateProxyClass(ClassMetaData& thisClass)
        {
            const clang::CXXRecordDecl& decl = thisClass.decl;
            clang::ASTContext& context = thisClass.context;

            // replace class name
            rewriter.replace(decl.getLocation(), thisClass.name + "_proxy");

            // remove original constructors
            for (auto ctor : decl.ctors())
            {
                rewriter.replace(ctor->getSourceRange(), "// constructor: removed");    
            }

            // insert constructor
            const std::string constructor = createProxyClassStandardConstructor(thisClass, decl) + "\n";
            if (thisClass.publicConstructors.size() > 0)
            {
                rewriter.replace(thisClass.publicConstructors[0].decl.getSourceRange(), constructor);
            }
            else
            {
                const clang::SourceLocation location = thisClass.publicScopeStart();
                rewriter.insert(location, "\n\t" + constructor, true, true);
            }

            // public variables: add reference qualifier
            addReferenceQualifierToPublicFields(thisClass, rewriter);
        }

        virtual void addReferenceQualifierToPublicFields(ClassMetaData& , Rewriter& rewriter) = 0;

    public:
        
        ClassDefinition(Rewriter& rewriter)
            :
            rewriter(rewriter)
        { ; }
    };

    class CXXClassDefinition : public ClassDefinition
    {
        using Base = ClassDefinition;
        using Base::rewriter;
        using Base::targetClasses;

        virtual void addReferenceQualifierToPublicFields(ClassMetaData& thisClass, Rewriter& rewriter)
        {
            using namespace clang::ast_matchers;
            
            rewriter.addMatcher(fieldDecl(allOf(isPublic(), hasParent(cxxRecordDecl(hasName(thisClass.name))))).bind("fieldDecl"),
                [] (const MatchFinder::MatchResult& result, Rewriter& rewriter)
                { 
                    if (const clang::FieldDecl* decl = result.Nodes.getNodeAs<clang::FieldDecl>("fieldDecl"))
                    {        
                        rewriter.replace(decl->getSourceRange(), decl->getType().getAsString() + "& " + decl->getNameAsString());
                    }
                });
            rewriter.run(thisClass.context);
            rewriter.clear();
        }

    public:
        
        CXXClassDefinition(Rewriter& rewriter)
            :
            Base(rewriter)
        { ; }

        virtual void run(const clang::ast_matchers::MatchFinder::MatchResult& result)
        {
            if (const clang::CXXRecordDecl* decl = result.Nodes.getNodeAs<clang::CXXRecordDecl>("classDecl"))
            {
                targetClasses.emplace_back(*decl, *result.Context, false);
                ClassMetaData& thisClass = targetClasses.back();

                if (thisClass.isProxyClassCandidate)
                {      
                    const std::string original_class = dumpDeclToStringHumanReadable(decl, rewriter.getLangOpts(), false);
                    Base::generateProxyClass(thisClass);
                    rewriter.insert(decl->getSourceRange().getBegin(), original_class + ";\n\n", true, true);
                }
                else
                {
                    targetClasses.pop_back();
                }
            }
        }
    };

    class ClassTemplateDefinition : public ClassDefinition
    {
        using Base = ClassDefinition;
        using Base::rewriter;
        using Base::targetClasses;

        virtual void addReferenceQualifierToPublicFields(ClassMetaData& thisClass, Rewriter& rewriter)
        {
            using namespace clang::ast_matchers;
            
            rewriter.addMatcher(fieldDecl(allOf(isPublic(), hasParent(cxxRecordDecl(allOf(hasName(thisClass.name), unless(isTemplateInstantiation())))))).bind("fieldDecl"),
                [] (const MatchFinder::MatchResult& result, Rewriter& rewriter)
                { 
                    if (const clang::FieldDecl* decl = result.Nodes.getNodeAs<clang::FieldDecl>("fieldDecl"))
                    {        
                        rewriter.replace(decl->getSourceRange(), decl->getType().getAsString() + "& " + decl->getNameAsString());
                    }
                });
            rewriter.run(thisClass.context);
            rewriter.clear();
        }

    public:
        
        ClassTemplateDefinition(Rewriter& rewriter)
            :
            Base(rewriter)
        { ; }
        
        virtual void run(const clang::ast_matchers::MatchFinder::MatchResult& result)
        {
            if (const clang::ClassTemplateDecl* decl = result.Nodes.getNodeAs<clang::ClassTemplateDecl>("classTemplateDecl"))
            {
                const bool isTemplatedDefinition = decl->isThisDeclarationADefinition();
                if (!isTemplatedDefinition) return;

                targetClasses.emplace_back(*(decl->getTemplatedDecl()), *result.Context, true);
                ClassMetaData& thisClass = targetClasses.back();
                
                if (thisClass.isProxyClassCandidate)
                {
                    const std::string original_class = dumpDeclToStringHumanReadable(decl, rewriter.getLangOpts(), false);
                    Base::generateProxyClass(thisClass);
                    rewriter.insert(decl->getSourceRange().getBegin(), original_class + ";\n\n", true, true);
                }
                else
                {
                    targetClasses.pop_back();
                }
            }
        }
    };
    */
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

/*
            FIX THAT
            matcher.addMatcher(varDecl(hasType(cxxRecordDecl(anyOf(hasName("vector"), hasName("array"))))).bind("varDecl"),
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
*/
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

            for (const auto& declaration : declarations)
            {
                declaration->printInfo(context.getSourceManager());
            }

            return (declarations.size() > 0);
        }

        bool findProxyClassTargets(clang::ASTContext& context)
        {
            using namespace clang::ast_matchers;

            Matcher matcher;
/*
            REMOVE: JUST FOR TESTING

            std::cout << "######################## HERE ##########################" << std::endl;
            //matcher.addMatcher(fieldDecl(allOf(isPublic, hasName("x"), hasParent(recordDecl().bind("targetDecl")))),
            matcher.addMatcher(fieldDecl(allOf(isPublic(), hasName("x"), hasParent(recordDecl().bind("targetDecl")))),
                [] (const MatchFinder::MatchResult& result) mutable
                {
                    if (const clang::RecordDecl* const decl = result.Nodes.getNodeAs<clang::RecordDecl>("targetDecl"))
                    {
                        std::cout << "record name: " << decl->getNameAsString() << std::endl;
                    }
                });
            matcher.run(context);

            std::cout << "######################## HERE ##########################" << std::endl;

            return false;
*/
            for (auto name : proxyClassTargetNames)
            {
                // template class declarations
                matcher.addMatcher(classTemplateDecl(hasName(name)).bind("classTemplateDeclaration"),
                    [this] (const MatchFinder::MatchResult& result) mutable
                    {
                        if (const clang::ClassTemplateDecl* const decl = result.Nodes.getNodeAs<clang::ClassTemplateDecl>("classTemplateDeclaration"))
                        {
                            if (decl->isThisDeclarationADefinition()) return;

                            proxyClassTargets.emplace_back(new TemplateClassMetaData(*decl, false));
                        }
                    });
                
                // template class definitions
                matcher.addMatcher(classTemplateDecl(hasName(name)).bind("classTemplateDefinition"),
                    [this] (const MatchFinder::MatchResult& result) mutable
                    {
                        if (const clang::ClassTemplateDecl* const decl = result.Nodes.getNodeAs<clang::ClassTemplateDecl>("classTemplateDefinition"))
                        {
                            if (!decl->isThisDeclarationADefinition()) return;

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
                    [this] (const MatchFinder::MatchResult& result) mutable
                    {
                        if (const clang::ClassTemplatePartialSpecializationDecl* const decl = result.Nodes.getNodeAs<clang::ClassTemplatePartialSpecializationDecl>("classTemplatePartialSpecialization"))
                        {
                            if (!isThisClassInstantiated(decl)) return;
                            
                            for (std::size_t i = 0; i < proxyClassTargets.size(); ++i)
                            {
                                if (proxyClassTargets[i]->addDefinition(*decl, true)) return;
                            }
                        }
                    });
                    
                // standard C++ classes
                matcher.addMatcher(cxxRecordDecl(allOf(hasName(name), unless(isTemplateInstantiation()))).bind("c++Class"),
                    [this] (const MatchFinder::MatchResult& result) mutable
                    {
                        if (const clang::CXXRecordDecl* const decl = result.Nodes.getNodeAs<clang::CXXRecordDecl>("c++Class"))
                        {
                            const std::string name = decl->getNameAsString();
                            const bool isDefinition = decl->isThisDeclarationADefinition();

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
            const std::uint32_t numTemplateParameters = definition.declaration.templateParameters.size();
            std::stringstream constructorDefinition;

            if (numTemplateParameters > 0)
            {
                constructorDefinition << indent << "template " << definition.declaration.templateParameterDeclStringX << "\n";
            }

            // signature without the parameter name
            constructorDefinition << indent << definition.name << "(const " << proxyNamespace << "::" << definition.name << "_proxy" << definition.declaration.templateParameterStringX <<  "& ";

            if (definition.hasCopyConstructor)
            {
                const clang::CXXConstructorDecl& constructorDecl = definition.constructors[definition.indexCopyConstructor].decl;
                const clang::ParmVarDecl* const parameter = constructorDecl.getParamDecl(0); // never nullptr

                // dump the definition of the copy constructor starting at the parameter name
                const clang::SourceRange everythingFromParmVarDeclName(parameter->getEndLoc(), constructorDecl.getEndLoc());
                constructorDefinition << dumpSourceRangeToString(everythingFromParmVarDeclName, definition.getSourceManager());
                if (constructorDecl.hasBody())
                {
                    constructorDefinition << "}";
                }
            }
            else
            {
                // parameter name + initializer list + no body
                constructorDefinition << "other)\n";
                constructorDefinition << generateConstructorInitializerList(definition, std::string("other"), insideClassIndent + 1);
                constructorDefinition << indent << "{ ; }";
            }
            constructorDefinition << "\n";

            return constructorDefinition.str();
        }
/*
        std::string createMethodWithProxyArguments(const internal::ClassMetaData::CXXMethod& method)
        {
            clang::ASTContext& context = method.decl.getASTContext();
            const clang::SourceManager& sourceManager = context.getSourceManager();
            std::string methodString;

            if (method.isMacroExpansion)
            {
                methodString = dumpSourceRangeToString(getExpansionSourceRange(method.sourceRange, context), sourceManager);
            }
            else
            {
                methodString = dumpSourceRangeToString(getExpansionSourceRange(method.sourceRange, context), sourceManager);
            }

            return (methodString + std::string("\n"));
        }
*/

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
                    #if !defined(bla)
                    if (method.hasClassTypeArguments)
                    { 
                        const std::string proxyTypeName = proxyNamespace + std::string("::") + method.className + std::string("_proxy");

                        if (method.isMacroExpansion)
                        {
                            std::string macroString = dumpSourceRangeToString(getExpansionSourceRange(method.sourceRange, context), sourceManager);                            
                            if (findAndReplace(macroString, method.className, proxyTypeName, true, true))
                            {
                                rewriter.insert(getNextLine(getExpansionSourceLocation(method.sourceRange.getEnd(), context), context), indent + macroString + std::string("\n"));
                            }
                        }
                        else
                        {
                            // duplicate the original function definition
                            std::string functionDefinition = dumpSourceRangeToString(method.extendedSourceRange, sourceManager);                            
                            rewriter.insert(getNextLine(method.sourceRange.getEnd(), context), indent + functionDefinition);

                            // apply changes to the original function definition: replace argument types by proxy types
                            for (const auto& classTypeArgument : method.indexClassTypeArguments)
                            {
                                const auto& argument = method.arguments[classTypeArgument];
                                std::string typeName = argument.elementTypeName;

                                if (argument.decl.hasDefaultArg())
                                {
                                    if (method.isDefinition)
                                    {
                                        const std::string variablDeclaration = extIndent + (argument.isConst ? "const " : "") + typeName + std::string(" ") +
                                            argument.name + std::string(" = ") + dumpStmtToStringHumanReadable(argument.decl.getDefaultArg(), false) + std::string(";\n");

                                        rewriter.insert(method.bodyInnerLocationBegin, variablDeclaration);
                                    }
                                }
                                else
                                {
                                    findAndReplace(typeName, method.className, std::string(proxyNamespace) + std::string("::") + method.className + std::string("_proxy"), true, true);

                                    const std::string argumentName = (argument.isConst ? "const " : "") + typeName +
                                        std::string(argument.isRReference ? "&&" : "") + 
                                        std::string(argument.isLReference ? "&" : "") + 
                                        std::string(argument.isPointer ? "* " : " ") + 
                                        argument.name;

                                    rewriter.replace(argument.sourceRange, argumentName);
                                }
                            }
                        }
                    }
                    #endif
                }
            }
            
            // insert include line outside the outermost namespace or after the last class definition if there is no namespace
            std::stringstream includeLine;
            includeLine << "\n#include \"autogen_" << candidate->name << "_proxy.hpp\"\n";
            rewriter.insert(getNextLine(candidate->bottomMostSourceLocation, context), includeLine.str());
        }

        std::string generateProxyClassConstructor(const ClassMetaData::Definition& definition, const std::string indent = std::string(""))
        {
            std::stringstream constructor;
            constructor << indent << definition.name << "_proxy(";
            constructor << definition.fields[0].typeName << "* ptr, const std::size_t n";
            if (definition.fields.size() > definition.indexPublicFields.size())
            {
                for (const auto& field : definition.fields)
                {
                    if (!field.isPublic)
                    {
                        constructor <<  ", " << field.typeName << " " << field.name;
                    }
                }
            }
            constructor << ")\n";
            constructor << indent << ":\n";

            const std::uint32_t numFields = definition.fields.size();
            std::uint32_t publicFieldId = 0;
            std::uint32_t fieldId = 0;
            for (const auto& field : definition.fields)
            {
                if (field.isPublic)
                {
                    constructor << indent << field.name << "(ptr[" << std::to_string(publicFieldId) <<" * n]";
                    ++publicFieldId;
                }
                else
                {
                    constructor << indent << field.name << "(" << field.name;
                }

                constructor << ((fieldId + 1) == numFields ? ")," : ")") << "\n";
                ++fieldId;
            }
            constructor << indent << "{ ; }";
            
            return constructor.str();
        }

        std::string generateProxyClassCopyConstructor(const ClassMetaData::Definition& definition, const std::string indent = std::string(""))
        {
            std::stringstream constructor;

            constructor << indent << definition.name << "_proxy(" << definition.name << "& rhs)\n";
            constructor << indent << generateConstructorInitializerList(definition, std::string("rhs"), Indentation(0, 0)) << "\n";
            constructor << indent << "{ ; }";

            return constructor.str();
        }

        void generateProxyClassDefinition(const std::unique_ptr<ClassMetaData>& candidate, Rewriter& rewriter, const std::string header = std::string(""))
        {
            if (!candidate.get()) return;

            // some file related locations
            const clang::SourceManager& sourceManager = candidate->sourceManager;
            const clang::FileID fileId = candidate->getDeclaration().fileId;
            const clang::SourceLocation fileStart = sourceManager.getLocForStartOfFile(fileId);
            const clang::SourceLocation fileEnd = sourceManager.getLocForEndOfFile(fileId);

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
                if (toBeRemoved.isValid())
                {
                    rewriter.remove(toBeRemoved);
                }
            }            

            const clang::SourceRange toBeRemoved(sourceRange->getEnd().getLocWithOffset(1), fileEnd);
            if (toBeRemoved.isValid())
            {
                rewriter.replace(toBeRemoved, std::string("\n"));
            }

            // declaration: replace class name by proxy class name
            const ClassMetaData::Declaration& declaration = candidate->getDeclaration();
            if (!declaration.isDefinition)
            {
                rewriter.replace(declaration.nameSourceRange, declaration.name + std::string("_proxy"));
            }

            // definitions
            const std::vector<ClassMetaData::Definition>& definitions = candidate->getDefinitions();
            for (const auto& definition : definitions)
            {
                // replace class name by proxy class name
                rewriter.replace(definition.nameSourceRange, definition.name + std::string("_proxy"));

                continue;

                // insert using stmt
                std::stringstream usingStmt;
                if (definition.isTemplatePartialSpecialization)
                {
                    for (const auto& templateArgument : definition.templatePartialSpecializationArguments)
                    {
                        const std::string templateArgumentName = templateArgument.first;
                        const bool isTypeParameter = templateArgument.second;
                        if (isTypeParameter)
                        {
                            usingStmt << "using nonconst_" << templateArgumentName << " = typename std::remove_const<" << templateArgumentName << ">::type;\n\t";
                        }
                    }
                }
                else
                {
                    for (const auto& templateParameter : definition.declaration.templateParameters)
                    {
                        const std::string templateArgumentName = templateParameter.name;
                        const bool isTemplateTypeParmType = templateParameter.isTypeParameter;
                        if (isTemplateTypeParmType)
                        {
                            usingStmt << "using nonconst_" << templateArgumentName << " = typename std::remove_const<" << templateArgumentName << ">::type;\n\t";
                        }
                    }
                }
                usingStmt << "using " << definition.name << " = " << definition.declaration.namespaceString << definition.name;
                if (definition.isTemplatePartialSpecialization)
                {
                    usingStmt << "<" << concat(definition.getTemplatePartialSpecializationArgumentNames("nonconst_"), std::string(", ")) << ">";
                }
                else if (definition.declaration.templateParameters.size() > 0)
                {
                    usingStmt << "<" << concat(definition.declaration.getTemplateParameterNames("nonconst_"), std::string(", ")) << ">";
                }
                usingStmt << ";";
                rewriter.insert(definition.innerLocBegin, std::string("\n\t") + usingStmt.str(), true, true);
               
                // standard constructor
                std::vector<std::string> proxyClassConstructors;
                proxyClassConstructors.emplace_back(generateProxyClassConstructor(definition, std::string("\t")));
                proxyClassConstructors.emplace_back(generateProxyClassCopyConstructor(definition, std::string("\t")));

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
            }
        }
        
        void addProxyClassToSource(const std::vector<std::unique_ptr<ClassMetaData>>& proxyClassTargets, clang::ASTContext& context)
        {
            // make a hard copy before applying any changes to the source code
            Rewriter proxyClassCreator(rewriter);

            for (const auto& target : proxyClassTargets)
            {
                if (!target->containsProxyClassCandidates) continue;

                std::cout << "########################################################" << std::endl;

                target->printInfo();
                
                // modify original source code
                modifyOriginalSourceCode(target, rewriter);

                // TODO: replace by writing back to file!
                rewriter.getEditBuffer(target->fileId).write(llvm::outs());
                
                std::cout << "########################################################" << std::endl;
                /*
                // generate proxy class definition
                generateProxyClassDefinition(target, proxyClassCreator, std::string("// my header\n"));

                // TODO: replace by writing back to file!
                proxyClassCreator.getEditBuffer(target->fileId).write(llvm::outs());

                std::cout << "########################################################" << std::endl;
                */
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