// Copyright (c) 2017-2018 Florian Wende (flwende@gmail.com)
//
// Distributed under the BSD 2-clause Software License
// (See accompanying file LICENSE)

#if !defined(TRAFO_DATA_LAYOUT_PROXY_GEN_HPP)
#define TRAFO_DATA_LAYOUT_PROXY_GEN_HPP

#include <iostream>
#include <string>
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
        //CXXClassDefinition cxxClassHandler;
        //ClassTemplateDefinition classTemplateHandler;
        
        std::vector<ContainerDeclaration> containerDeclarations;
        std::set<std::string> proxyClassCandidateNames;
        std::vector<std::unique_ptr<ClassMetaData>> proxyClassCandidates;
        
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

    public:
        
        InsertProxyClassImplementation(clang::Rewriter& clangRewriter) 
            :
            rewriter(clangRewriter)//,
            //cxxClassHandler(rewriter),
            //classTemplateHandler(rewriter)
        { ; }

        void HandleTranslationUnit(clang::ASTContext& context) override
        {	
            using namespace clang::ast_matchers;
            
            Matcher matcher;

            // step 1: find all relevant container declarations
            std::vector<std::string> containerNames;
            containerNames.push_back("vector");

            /*
            std::vector<std::string> cns;
            cns.push_back("A");
            cns.push_back("B");
            cns.push_back("C");

            for (auto cn : cns)
            {
                matcher.addMatcher(cxxRecordDecl(allOf(hasName(cn), isDefinition())).bind("classDecl"),
                    [&] (const MatchFinder::MatchResult& result) mutable
                    {
                        if (const clang::CXXRecordDecl* decl = result.Nodes.getNodeAs<clang::CXXRecordDecl>("classDecl"))
                        {
                            std::cout << "found " << decl->getNameAsString() << std::endl;
                            if (Matcher::testDecl(*decl, cxxRecordDecl(isTemplateInstantiation()), context))
                                std::cout << "\ttemplate instantiation" << std::endl;
                            if (Matcher::testDecl(*decl, cxxRecordDecl(isTemplateInstantiation()), context))
                            std::cout << "\trange: " << decl->getSourceRange().printToString(rewriter.getSourceMgr()) << std::endl;
                        }
                    });

                matcher.addMatcher(classTemplateDecl(hasName(cn)).bind("classDecl_0"),
                    [&] (const MatchFinder::MatchResult& result) mutable
                    {
                        if (const clang::ClassTemplateDecl* decl = result.Nodes.getNodeAs<clang::ClassTemplateDecl>("classDecl_0"))
                        {
                            if (decl->isThisDeclarationADefinition()) return;

                            std::cout << "found DECLARATION " << decl->getNameAsString() << std::endl;
                            std::cout << "\trange: " << decl->getSourceRange().printToString(rewriter.getSourceMgr()) << std::endl;
                        }
                    });

                matcher.addMatcher(classTemplateDecl(hasName(cn)).bind("classDecl_1"),
                    [&] (const MatchFinder::MatchResult& result) mutable
                    {
                        if (const clang::ClassTemplateDecl* decl = result.Nodes.getNodeAs<clang::ClassTemplateDecl>("classDecl_1"))
                        {
                            if (!decl->isThisDeclarationADefinition()) return;

                            std::cout << "found DEFINITION " << decl->getNameAsString() << std::endl;
                            std::cout << "\trange: " << decl->getSourceRange().printToString(rewriter.getSourceMgr()) << std::endl;
                        }
                    });

                matcher.addMatcher(classTemplatePartialSpecializationDecl(hasName(cn)).bind("classDecl_2"),
                    [&] (const MatchFinder::MatchResult& result) mutable
                    {
                        if (const clang::ClassTemplatePartialSpecializationDecl* decl = result.Nodes.getNodeAs<clang::ClassTemplatePartialSpecializationDecl>("classDecl_2"))
                        {
                            std::cout << "found PARTIAL SPECIALIZATION " << decl->getNameAsString() << std::endl;
                            std::cout << "\trange: " << decl->getSourceRange().printToString(rewriter.getSourceMgr()) << std::endl;
                        }
                    });

                matcher.addMatcher(classTemplateSpecializationDecl(hasName(cn)).bind("classDecl_3"),
                    [&] (const MatchFinder::MatchResult& result) mutable
                    {
                        if (const clang::ClassTemplateSpecializationDecl* decl = result.Nodes.getNodeAs<clang::ClassTemplateSpecializationDecl>("classDecl_3"))
                        {
                            std::cout << "found SPECIALIZATION " << decl->getNameAsString() << std::endl;
                            std::cout << "\trange: " << decl->getSourceRange().printToString(rewriter.getSourceMgr()) << std::endl;
                        }
                    });

                matcher.addMatcher(cxxRecordDecl(allOf(hasName(cn), isDefinition(), isInstantiated())).bind("classDecl_4"),
                    [&] (const MatchFinder::MatchResult& result) mutable
                    {
                        if (const clang::CXXRecordDecl* decl = result.Nodes.getNodeAs<clang::CXXRecordDecl>("classDecl_4"))
                        {
                            std::cout << "found INSTANTIATION " << decl->getNameAsString() << std::endl;
                            std::cout << "\trange: " << decl->getSourceRange().printToString(rewriter.getSourceMgr()) << std::endl;

                            Matcher localMatcher;
                            localMatcher.addMatcher(fieldDecl(isPublic()).bind("fieldDecl"),
                                [&] (const MatchFinder::MatchResult& result) mutable
                                {
                                    if (const clang::FieldDecl* fdecl = result.Nodes.getNodeAs<clang::FieldDecl>("fieldDecl"))
                                    {                             
                                        std::cout << "\t\tfield: " << fdecl->getNameAsString() << std::endl;
                                        std::cout << "\t\t\trange: " << fdecl->getSourceRange().printToString(rewriter.getSourceMgr()) << std::endl;
                                    }
                                }, decl);
                            localMatcher.run(decl->getASTContext());
                            localMatcher.clear();
                        }
                    });

                matcher.addMatcher(cxxRecordDecl(allOf(hasName(cn), isDefinition(), unless(isTemplateInstantiation()))).bind("classDecl_5"),
                    [&] (const MatchFinder::MatchResult& result) mutable
                    {
                        if (const clang::CXXRecordDecl* decl = result.Nodes.getNodeAs<clang::CXXRecordDecl>("classDecl_5"))
                        {
                            std::cout << "found C++ class " << decl->getNameAsString() << std::endl;
                            std::cout << "\trange: " << decl->getSourceRange().printToString(rewriter.getSourceMgr()) << std::endl;
                        }
                    });
            }
            matcher.run(context);
            matcher.clear();

            return;
            */
            for (auto containerName : containerNames)
            {
                matcher.addMatcher(varDecl(hasType(cxxRecordDecl(hasName(containerName)))).bind("varDecl"),
                    [&] (const MatchFinder::MatchResult& result) mutable
                    {
                        if (const clang::VarDecl* decl = result.Nodes.getNodeAs<clang::VarDecl>("varDecl"))
                        {
                            ContainerDeclaration vecDecl = ContainerDeclaration::make(*decl, context, containerName);
                            if (!vecDecl.elementDataType.isNull() && !vecDecl.elementDataType.isTrivialType(context))
                            {
                                containerDeclarations.push_back(vecDecl);
                                proxyClassCandidateNames.insert(vecDecl.elementDataTypeName);
                            }
                        }
                    });
            }
            matcher.run(context);
            matcher.clear();
            
            //clang::NamespaceDecl* namespaceDecl;

            // step 2: check if element data type is candidate for proxy class generation
            for (auto className : proxyClassCandidateNames)
            {
                /*
                //matcher.addMatcher(namespaceDecl(allOf(hasName("fww"), hasDescendant(namedDecl(hasName(className))))).bind("namespaceDecl"),
                matcher.addMatcher(namespaceDecl(hasDescendant(namedDecl(hasName(className)))).bind("namespaceDecl"),
                    [&] (const MatchFinder::MatchResult& result) mutable
                    {
                        if (const clang::NamespaceDecl* decl = result.Nodes.getNodeAs<clang::NamespaceDecl>("namespaceDecl"))
                        {
                            std::cout << "FOUND NAMESPACE: " << decl->getNameAsString() << std::endl;
                            std::cout << "\trange: " << decl->getSourceRange().printToString(rewriter.getSourceMgr()) << std::endl;
                        }
                    });
                    */

                // template class declarations
                matcher.addMatcher(classTemplateDecl(hasName(className)).bind("classTemplateDeclaration"),
                    [&] (const MatchFinder::MatchResult& result) mutable
                    {
                        if (const clang::ClassTemplateDecl* decl = result.Nodes.getNodeAs<clang::ClassTemplateDecl>("classTemplateDeclaration"))
                        {
                            if (decl->isThisDeclarationADefinition()) return;
                            proxyClassCandidates.emplace_back(new TemplateClassMetaData(*decl, false));
                        }
                    });
                
                // template class definitions
                matcher.addMatcher(classTemplateDecl(hasName(className)).bind("classTemplateDefinition"),
                    [&] (const MatchFinder::MatchResult& result) mutable
                    {
                        if (const clang::ClassTemplateDecl* decl = result.Nodes.getNodeAs<clang::ClassTemplateDecl>("classTemplateDefinition"))
                        {
                            if (!decl->isThisDeclarationADefinition()) return;

                            for (std::size_t i = 0; i < proxyClassCandidates.size(); ++i)
                            {
                                if (proxyClassCandidates[i]->addDefinition(*(decl->getTemplatedDecl()))) return;
                            }
                                       
                            // not found
                            proxyClassCandidates.emplace_back(new TemplateClassMetaData(*decl, true));
                            proxyClassCandidates.back()->addDefinition(*(decl->getTemplatedDecl()));
                        }
                    });


                // template class partial specialization
                matcher.addMatcher(classTemplatePartialSpecializationDecl(hasName(className)).bind("classTemplatePartialSpecialization"),
                    [&] (const MatchFinder::MatchResult& result) mutable
                    {
                        if (const clang::ClassTemplatePartialSpecializationDecl* decl = result.Nodes.getNodeAs<clang::ClassTemplatePartialSpecializationDecl>("classTemplatePartialSpecialization"))
                        {
                            if (!isThisClassInstantiated(decl)) return;
                            
                            for (std::size_t i = 0; i < proxyClassCandidates.size(); ++i)
                            {
                                if (proxyClassCandidates[i]->addDefinition(*decl, true)) return;
                            }
                        }
                    });
                    
                // standard C++ classes
                matcher.addMatcher(cxxRecordDecl(allOf(hasName(className), isDefinition(), unless(isTemplateInstantiation()))).bind("c++Class"),
                    [&] (const MatchFinder::MatchResult& result) mutable
                    {
                        if (const clang::CXXRecordDecl* decl = result.Nodes.getNodeAs<clang::CXXRecordDecl>("c++Class"))
                        {
                            const std::string className = decl->getNameAsString();
                            for (std::size_t i = 0; i < proxyClassCandidates.size(); ++i)
                            {
                                // if it is a specialization of a template class: skip this class definition
                                if (proxyClassCandidates[i]->name == className && proxyClassCandidates[i]->isTemplated) return;
                            }
                            
                            proxyClassCandidates.emplace_back(new CXXClassMetaData(*decl, true));
                            proxyClassCandidates.back()->addDefinition(*decl, false);
                        }
                    });
            }
            matcher.run(context);
            matcher.clear();

            /*
            // step 3: generate proxy classes
            for (auto thisClass : targetClasses)
            {
                // backup the original buffer content
                clang::RewriteBuffer& rewriteBuffer = rewriter.getEditBuffer(rewriter.getSourceMgr().getFileID(thisClass.decl.getSourceRange().getBegin()));
                std::string originalBufferStr;
                llvm::raw_string_ostream originalBuffer(originalBufferStr);
                rewriteBuffer.write(originalBuffer);

                rewriter.insert(thisClass.decl.getSourceRange().getBegin(), "\n\nSOME TEXT\n\n", true, true);            
                rewriteBuffer.write(llvm::outs());

                rewriteBuffer.Initialize(originalBuffer.str());
                rewriteBuffer.write(llvm::outs());
            }
            */

            for (std::size_t i = 0; i < proxyClassCandidates.size(); ++i)
            {
                proxyClassCandidates[i]->printInfo(rewriter.getSourceMgr());
            }
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