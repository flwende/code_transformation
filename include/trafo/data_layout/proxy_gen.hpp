// Copyright (c) 2017-2018 Florian Wende (flwende@gmail.com)
//
// Distributed under the BSD 2-clause Software License
// (See accompanying file LICENSE)

#if !defined(TRAFO_DATA_LAYOUT_PROXY_GEN_HPP)
#define TRAFO_DATA_LAYOUT_PROXY_GEN_HPP

#include <iostream>
#include <cstdint>
#include <memory>
#include <vector>
#include <set>

#include <clang/AST/AST.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/DeclTemplate.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>

#include <misc/string_helper.hpp>
#include <misc/rewriter.hpp>
#include <misc/matcher.hpp>
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
        
        std::vector<ContainerDeclaration> containerDeclarations;
        std::set<std::string> proxyClassTargetNames;
        std::vector<std::unique_ptr<ClassMetaData>> proxyClassTargets;
        
        bool isThisClassInstantiated(const clang::CXXRecordDecl* decl)
        {
            using namespace clang::ast_matchers;

            bool testResult = false;
            if (decl == nullptr) return testResult;

            Matcher matcher;
            matcher.addMatcher(cxxRecordDecl(allOf(hasName(decl->getNameAsString()), isInstantiated())).bind("test"),
                [&] (const MatchFinder::MatchResult& result) mutable
                {
                    if (const clang::CXXRecordDecl* instance = result.Nodes.getNodeAs<clang::CXXRecordDecl>("test"))
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

        bool matchContainerDeclarations(const std::vector<std::string>& containerNames, clang::ASTContext& context)
        {
            using namespace clang::ast_matchers;

            Matcher matcher;

            containerDeclarations.clear();
            for (auto containerName : containerNames)
            {
                matcher.addMatcher(varDecl(hasType(cxxRecordDecl(hasName(containerName)))).bind("varDecl"),
                    [&] (const MatchFinder::MatchResult& result) mutable
                    {
                        if (const clang::VarDecl* decl = result.Nodes.getNodeAs<clang::VarDecl>("varDecl"))
                        {
                            ContainerDeclaration vecDecl = ContainerDeclaration::make(*decl, context, containerName);
                            const clang::Type* type = vecDecl.elementDataType.getTypePtrOrNull();
                            const bool isRecordType = (type != nullptr ? type->isRecordType() : false);
                            if (!vecDecl.elementDataType.isNull() && isRecordType)
                            {
                                containerDeclarations.push_back(vecDecl);
                                proxyClassTargetNames.insert(vecDecl.elementDataTypeName);
                            }
                        }
                    });
            }
            matcher.run(context);

            return (containerDeclarations.size() > 0);
        }

        bool findProxyClassTargets(clang::ASTContext& context)
        {
            using namespace clang::ast_matchers;

            Matcher matcher;

            for (auto name : proxyClassTargetNames)
            {
                // template class declarations
                matcher.addMatcher(classTemplateDecl(hasName(name)).bind("classTemplateDeclaration"),
                    [&] (const MatchFinder::MatchResult& result) mutable
                    {
                        if (const clang::ClassTemplateDecl* decl = result.Nodes.getNodeAs<clang::ClassTemplateDecl>("classTemplateDeclaration"))
                        {
                            if (decl->isThisDeclarationADefinition()) return;
                            proxyClassTargets.emplace_back(new TemplateClassMetaData(*decl, false));
                        }
                    });
                
                // template class definitions
                matcher.addMatcher(classTemplateDecl(hasName(name)).bind("classTemplateDefinition"),
                    [&] (const MatchFinder::MatchResult& result) mutable
                    {
                        if (const clang::ClassTemplateDecl* decl = result.Nodes.getNodeAs<clang::ClassTemplateDecl>("classTemplateDefinition"))
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
                    [&] (const MatchFinder::MatchResult& result) mutable
                    {
                        if (const clang::ClassTemplatePartialSpecializationDecl* decl = result.Nodes.getNodeAs<clang::ClassTemplatePartialSpecializationDecl>("classTemplatePartialSpecialization"))
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
                    [&] (const MatchFinder::MatchResult& result) mutable
                    {
                        if (const clang::CXXRecordDecl* decl = result.Nodes.getNodeAs<clang::CXXRecordDecl>("c++Class"))
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
        
        std::string generateProxyClassDeclaration(const ClassMetaData::Declaration& declaration, const std::string indent = std::string(""))
        {
            std::stringstream proxyClassDeclaration;
            proxyClassDeclaration << indent << "namespace proxy_internal" << std::endl << "{" << std::endl;
            if (declaration.templateParameters.size() > 0)
            {
                proxyClassDeclaration << indent << "\t" << dumpSourceRangeToString(declaration.templateParameterListSourceRange, declaration.getSourceManager());
                proxyClassDeclaration << indent << ">" << std::endl;
            }
            proxyClassDeclaration << indent << "\t" << (declaration.isStruct ? "struct " : "class ") << declaration.name << "_proxy;" << std::endl;
            proxyClassDeclaration << indent << "}";

            return proxyClassDeclaration.str();
        }

        std::string generateUsingStmt(const ClassMetaData::Definition& definition, const std::string indent = std::string(""))
        {
            std::stringstream usingStmt;
            usingStmt << indent << "using " << definition.name << "_proxy = proxy_internal::" << definition.name << "_proxy";
            if (definition.isTemplatePartialSpecialization)
            {
                usingStmt << "<" << concat(definition.templatePartialSpecializationArguments, std::string(", ")) << ">";
            }
            else if (definition.declaration.templateParameters.size() > 0)
            {
                usingStmt << "<" << concat(definition.declaration.getTemplateParameterNames(), std::string(", ")) << ">";
            }

            return usingStmt.str();
        }

        std::string generateConstructorClassFromProxyClass(const ClassMetaData::Definition& definition, const std::string indent = std::string(""))
        {
            std::stringstream constructorDefinition;

            if (definition.hasCopyConstructor)
            {
                const clang::CXXConstructorDecl& constructorDecl = definition.copyConstructor->decl;
                const clang::ParmVarDecl* parameter = constructorDecl.getParamDecl(0);

                // signature without the parameter name
                constructorDefinition << indent << definition.name << "(const " << definition.name << "_proxy& ";

                // dump the definition of the copy constructor starting at the parameter name
                clang::SourceRange everythingFromParmVarDeclName = clang::SourceRange(parameter->getEndLoc(), constructorDecl.getEndLoc());
                constructorDefinition << dumpSourceRangeToString(everythingFromParmVarDeclName, definition.getSourceManager());
                if (constructorDecl.hasBody())
                {
                    constructorDefinition << "}";
                }
            }
            else
            {
                // signature
                constructorDefinition << indent << definition.name << "(const " << definition.name << "_proxy& other) ";

                // initializer list
                if (const std::uint32_t numInitializers = definition.fields.size())
                {
                    constructorDefinition << ": ";
                    std::uint32_t initializerId = 0;
                    for (std::uint32_t i = 0; i < numInitializers; ++i)
                    {
                        const std::string fieldName = definition.fields[i].name;
                        constructorDefinition << fieldName << "(other." << fieldName;
                        constructorDefinition << (++initializerId < numInitializers ? "), " : ") ");
                    }
                }

                // no body!
                constructorDefinition << "{ ; }";
            }

            return constructorDefinition.str();
        }

        void modifyOriginalSourceCode(const std::unique_ptr<ClassMetaData>& candidate, Rewriter& rewriter)
        {
            const ClassMetaData::Declaration& declaration = candidate->getDeclaration();
            const std::vector<ClassMetaData::Definition>& definitions = candidate->getDefinitions();

            // insert proxy class forward declaration   
            rewriter.insert(declaration.sourceRange.getBegin(), generateProxyClassDeclaration(declaration) + std::string("\n\n"), true, true);

            // add constructors with proxy class arguments
            for (auto& definition : definitions)
            {
                // insert using statement into class definition
                rewriter.insert(definition.innerLocBegin, std::string("\n") + generateUsingStmt(definition, "\t") + std::string(";"), true, true);
                
                // insert constructor: in case of a C++ class, there must be at least one public access specifier
                // as we require proxy class candidates have at least one public field!
                std::string prefix("\n");
                clang::SourceLocation sourceLocation;
                if (definition.hasCopyConstructor)
                {
                    sourceLocation = definition.copyConstructor->sourceRange.getEnd().getLocWithOffset(1);
                }
                else if (definition.ptrPublicConstructors.size() > 0)
                {
                    sourceLocation = definition.ptrPublicConstructors.back()->sourceRange.getEnd().getLocWithOffset(1);
                }
                else if (definition.publicAccess)
                {
                    sourceLocation = definition.publicAccess->scopeBegin;
                    prefix += std::string("\t");

                }
                else if (definition.declaration.isStruct)
                {
                    sourceLocation = definition.innerLocBegin;
                    prefix += std::string("\t");
                }
                rewriter.insert(sourceLocation, prefix + generateConstructorClassFromProxyClass(definition), true, true);
            }

            // insert include line
            std::stringstream includeLine;
            includeLine << std::string("#include \"autogen_") << candidate->name << "_proxy.hpp\"";
            // insert outside the outermost namespace or after the last class definition if there is no namespace
            clang::SourceLocation sourceLocation;
            if (declaration.namespaces.size() > 0)
            {
                sourceLocation = declaration.namespaces.front().sourceRange.getEnd().getLocWithOffset(1);
            }
            else
            {
                sourceLocation = definitions.back().sourceRange.getEnd().getLocWithOffset(1);
            }
            // insert
            rewriter.insert(sourceLocation, std::string("\n\n") + includeLine.str(), true, true);
        }

        void generateProxyClassDefinition(const std::unique_ptr<ClassMetaData>& candidate, Rewriter& rewriter, clang::ASTContext& context, const std::string header = std::string(""))
        {
            const clang::SourceManager& sourceManager = context.getSourceManager();
            const ClassMetaData::Declaration& declaration = candidate->getDeclaration();
            const std::vector<ClassMetaData::Definition>& definitions = candidate->getDefinitions();

            // remove everything outside the outer namespace
            clang::SourceLocation fileStart = sourceManager.getLocForStartOfFile(sourceManager.getFileID(candidate->getSourceLocation()));
            clang::SourceLocation fileEnd = sourceManager.getLocForEndOfFile(sourceManager.getFileID(candidate->getSourceLocation()));
            
            const std::string headerSepSpace = std::string(header.empty() ? "" : "\n\n");
            if (declaration.namespaces.size() > 0)
            {
                clang::SourceRange fileStartToDecl = clang::SourceRange(fileStart, declaration.namespaces.front().sourceRange.getBegin());
                clang::SourceRange declToFileEnd = clang::SourceRange(declaration.namespaces.front().sourceRange.getEnd().getLocWithOffset(1), fileEnd);
                rewriter.replace(fileStartToDecl, header + headerSepSpace + std::string("namespace"));
                rewriter.replace(declToFileEnd, std::string("\n"));
            }
            else
            {
                clang::SourceRange fileStartToDecl = clang::SourceRange(fileStart, candidate->getSourceLocation());
                clang::SourceRange declToFileEnd = clang::SourceRange(definitions.back().sourceRange.getEnd().getLocWithOffset(1), fileEnd);
                rewriter.replace(fileStartToDecl, header + headerSepSpace);
                rewriter.replace(declToFileEnd, std::string("\n"));
            }
        }
        
        void addProxyClassToSource(const std::vector<std::unique_ptr<ClassMetaData>>& proxyClassTargets, clang::ASTContext& context)
        {
            // make a hard copy before applying any changes to the source code
            Rewriter proxyClassCreator(rewriter);

            for (auto& target : proxyClassTargets)
            {
                if (!target->containsProxyClassCandidates) continue;

                // modify original source code
                modifyOriginalSourceCode(target, rewriter);

                // TODO: replace by writing back to file!
                rewriter.getEditBuffer(rewriter.getSourceManager().getFileID(target->getSourceLocation())).write(llvm::outs());

                std::cout << "########################################################" << std::endl;

                // generate proxy class definition
                generateProxyClassDefinition(target, proxyClassCreator, context, std::string("// my header"));

                // TODO: replace by writing back to file!
                proxyClassCreator.getEditBuffer(proxyClassCreator.getSourceManager().getFileID(target->getSourceLocation())).write(llvm::outs());
                
                std::cout << "########################################################" << std::endl;
            }
        }
        
    public:
        
        InsertProxyClassImplementation(clang::Rewriter& clangRewriter)
            :
            rewriter(clangRewriter)
        { ; }

        void HandleTranslationUnit(clang::ASTContext& context) override
        {	
            // step 1: find all relevant container declarations
            std::vector<std::string> containerNames;
            containerNames.push_back("vector");
            if (!matchContainerDeclarations(containerNames, context)) return;

            // step 2: check if element data type is candidate for proxy class generation
            if (!findProxyClassTargets(context)) return;

            // step 3: add proxy classes
            addProxyClassToSource(proxyClassTargets, context);

            #if defined(old)
            for (auto& candidate : proxyClassCandidates)
            {
                if (!candidate->containsProxyClassCandidates) continue;
        
                std::cout << "############################################################################################################################" << std::endl;
                std::cout << "### CANDIDATE: ";
                candidate->printInfo();
                std::cout << "############################################################################################################################" << std::endl;

                // backup the original buffer content
                clang::RewriteBuffer& rewriteBuffer = rewriter.getEditBuffer(rewriter.getSourceManager().getFileID(candidate->getSourceLocation()));
                std::string originalBufferStr;
                llvm::raw_string_ostream originalBuffer(originalBufferStr);
                rewriteBuffer.write(originalBuffer);

                /*
                // iterate over candidate definitions: 
                for (auto& definition : candidate->getDefinitions())
                {
                    rewriter.insert(definition.sourceRange.getBegin(), "TRANSFORM INTO PROXY\n", true, true);            
                    rewriteBuffer.write(llvm::outs());
                }
                */

                // insert proxy class forward declaration
                const ClassMetaData::Declaration& declaration = candidate->getDeclaration();
                rewriteBuffer.Initialize(originalBuffer.str());
                rewriter.insert(declaration.sourceRange.getBegin(), proxyClassDeclaration(declaration) + std::string("\n\n"), true, true);

                // add constructors with proxy class arguments
                for (auto& definition : candidate->getDefinitions())
                {
                    std::stringstream usingProxy;
                    usingProxy << "using " << definition.name << "_proxy = proxy_internal::" << definition.name << "_proxy";
                    if (definition.isTemplatePartialSpecialization)
                    {
                        usingProxy << "<" << concat(definition.templatePartialSpecializationArguments, std::string(", ")) << ">";
                    }
                    else if (candidate->isTemplated())
                    {
                        usingProxy << "<" << concat(definition.declaration.getTemplateParameterNames(), std::string(", ")) << ">";
                    }
                    std::cout << "USING: " << usingProxy.str() << std::endl;

                    const std::string constructor = constructorClassFromProxyClass(definition);
                    const clang::SourceLocation sourceLocation = (definition.hasCopyConstructor ? definition.copyConstructor->sourceRange 
                                                                                                : definition.constructors.back().sourceRange).getEnd().getLocWithOffset(1);
                    rewriter.insert(sourceLocation, std::string("\n") + constructor, true, true);
                }

                rewriteBuffer.write(llvm::outs());
            }
            #endif
        }
    };

    class InsertProxyClass : public clang::ASTFrontendAction
    {
        clang::Rewriter rewriter;
        
    public:
        
        InsertProxyClass() { ; }
        
        void EndSourceFileAction() override
        {
            //rewriter.getEditBuffer(rewriter.getSourceMgr().getMainFileID()).write(llvm::outs());
        }
        
        std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance& ci, llvm::StringRef file) override
        {
            rewriter.setSourceMgr(ci.getSourceManager(), ci.getLangOpts());
            return llvm::make_unique<InsertProxyClassImplementation>(rewriter);
        }
    };
}

#endif