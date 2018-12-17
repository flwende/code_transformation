// Copyright (c) 2017-2018 Florian Wende (flwende@gmail.com)
//
// Distributed under the BSD 2-clause Software License
// (See accompanying file LICENSE)

#if !defined(PROXY_GEN_HPP)
#define PROXY_GEN_HPP

#include <iostream>
#include <string>
#include <vector>

#include <clang/AST/AST.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/AST/DeclTemplate.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>

#include <misc/string_helper.hpp>

#if !defined(TRAFO_NAMESPACE)
    #define TRAFO_NAMESPACE fw
#endif

namespace TRAFO_NAMESPACE
{
    using namespace clang;
    using namespace clang::ast_matchers;
    using namespace clang::driver;
    using namespace clang::tooling;
    using namespace TRAFO_NAMESPACE::internal;

    class ClassDefinition : public MatchFinder::MatchCallback
    {
    protected:

        Rewriter& rewriter;

        // generic handler definitions
        class MyRewriter : public MatchFinder::MatchCallback
        {
            Rewriter& rewriter;
            std::function<void(const MatchFinder::MatchResult&, Rewriter&)> kernel;

        public:

            template <typename F>
            MyRewriter(Rewriter& rewriter, const F& kernel)
                :
                rewriter(rewriter),
                kernel(kernel)  
            { ; }
            
            virtual void run(const MatchFinder::MatchResult& result)
            {
                kernel(result, rewriter);
            }

            template <typename T>
            void matchAndModify(ASTContext& context, const T& match)
            {
                MatchFinder matcher;
                matcher.addMatcher(match, this);
                matcher.matchAST(context);
            }
        };

        template <typename T, typename F>
        void matchAndModify(ASTContext& context, const T& match, const F& kernel)
        {
            MyRewriter myRewriter(rewriter, kernel);
            MatchFinder matcher;
            matcher.addMatcher(match, &myRewriter);
            matcher.matchAST(context);
        }

        class MyMatchFinder : public MatchFinder::MatchCallback
        {
            std::function<void(const MatchFinder::MatchResult&)> kernel;

        public:

            template <typename F>
            MyMatchFinder(const F& kernel)
                :
                kernel(kernel)  
            { ; }
            
            virtual void run(const MatchFinder::MatchResult& result)
            {
                kernel(result);
            }

            template <typename T>
            void match(ASTContext& context, const T& match)
            {
                MatchFinder matcher;
                matcher.addMatcher(match, this);
                matcher.matchAST(context);
            }
        };

        template <typename T, typename F>
        void match(ASTContext& context, const T& match, const F& kernel) const
        {
            MyMatchFinder myMatcher(kernel);
            MatchFinder matcher;
            matcher.addMatcher(match, &myMatcher);
            matcher.matchAST(context);
        }

        // meta data
        class MetaData
        {
            struct MemberField
            {
                const FieldDecl& decl;
                const bool isConst;
                const bool isFundamentalOrTemplateType;
                const std::string typeName;
                const std::string name;
                MemberField(const FieldDecl& decl, const bool isConst, const bool isFundamentalOrTemplateType, const std::string typeName, const std::string name)
                    :
                    decl(decl),
                    isConst(isConst),
                    isFundamentalOrTemplateType(isFundamentalOrTemplateType),
                    typeName(typeName),
                    name(name)
                { ; }
            };

        public:

            std::string className;
            bool isTemplateClass;
            bool isTemplateClassDefinition;
            std::size_t numFields;
            std::vector<MemberField> publicFields;
            std::vector<MemberField> privateFields;
            bool hasNonFundamentalPublicFields;
            bool hasMultiplePublicFieldTypes;
            bool isProxyClassCandidate;
        } classInfo;

        // implementation
        void collectMetaInformation(const CXXRecordDecl& decl, ASTContext& context)
        {
            classInfo.className = decl.getNameAsString();

            // classInfo.isTemplateClass has been set already
            // classInfo.isTemplateClassDefinition has been set already

            // proxy class candidates should not have any of these properties: abstract, polymorphic, empty
            classInfo.isProxyClassCandidate = !(decl.isAbstract() || decl.isPolymorphic() || decl.isEmpty());
            // proxy class candidates must be class definitions and no specializations in case of template classes
            // note: a template class specialization is of type CXXRecordDecl, and we need to check for its describing template class
            //       (if it does not exist, then it is a normal C++ class)
            classInfo.isProxyClassCandidate &= (classInfo.isTemplateClass ? classInfo.isTemplateClassDefinition : (decl.getDescribedClassTemplate() == nullptr));
            if (!classInfo.isProxyClassCandidate) return;
                
            classInfo.numFields = std::distance(decl.field_begin(), decl.field_end());
            if (classInfo.numFields > 0)
            {
                // match all (public, private) fields of the class and collect information: declaration, const qualifier, fundamental type, type name, name
            #define MATCH(MODIFIER, VARIABLE) \
                match(context, classInfo.isTemplateClass ? fieldDecl(allOf(MODIFIER(), hasParent(cxxRecordDecl(allOf(hasName(classInfo.className), unless(isTemplateInstantiation())))))).bind("fieldDecl") \
                                                        : fieldDecl(allOf(MODIFIER(), hasParent(cxxRecordDecl(hasName(classInfo.className))))).bind("fieldDecl"), \
                    [&] (const MatchFinder::MatchResult& result) mutable \
                    { \
                        if (const FieldDecl* decl = result.Nodes.getNodeAs<FieldDecl>("fieldDecl")) \
                        { \
                            const QualType qualType = decl->getType(); \
                            const bool isConstant = qualType.getQualifiers().hasConst(); \
                            const Type* type = qualType.getTypePtrOrNull(); \
                            const bool isFundamentalOrTemplateType = (type != nullptr ? (type->isFundamentalType() || type->isTemplateTypeParmType()) : false); \
                            VARIABLE.emplace_back(*decl, isConstant, isFundamentalOrTemplateType, decl->getType().getAsString(), decl->getNameAsString()); \
                        } \
                    } \
                )
                
                MATCH(isPublic, classInfo.publicFields);
                classInfo.isProxyClassCandidate &= (classInfo.publicFields.size() > 0);
                // proxy class candidates must have at least 1 public field
                if (!classInfo.isProxyClassCandidate) return;
                MATCH(isPrivate, classInfo.privateFields);
            #undef MATCH
            }

            classInfo.hasNonFundamentalPublicFields = false;
            classInfo.hasMultiplePublicFieldTypes = false;
            const std::string typeName = classInfo.publicFields[0].typeName;
            for (auto field : classInfo.publicFields)
            {
                classInfo.hasNonFundamentalPublicFields |= !(field.isFundamentalOrTemplateType);
                classInfo.hasMultiplePublicFieldTypes |= (field.typeName != typeName);
            }
            // proxy class candidates must have fundamental public fields
            classInfo.isProxyClassCandidate &= !classInfo.hasNonFundamentalPublicFields;// || classInfo.hasMultiplePublicFieldTypes);
        }

        std::string createProxyClassStandardConstructor(const CXXRecordDecl& decl) const
        {
            // CONTINUE HERE
            std::string constructor = classInfo.className + "_proxy(";

            return constructor;
        }

        // generate proxy class definition
        void generateProxyClass(const CXXRecordDecl& decl, ASTContext& context)
        {
            // replace class name
            rewriter.ReplaceText(decl.getLocation(), classInfo.className + "_proxy");
                    
            // remove original constructors
            for (auto ctor : decl.ctors())
            {
                rewriter.ReplaceText(ctor->getSourceRange(), "// constructor: removed");    
            }

            // insert standard constructor
            createProxyClassStandardConstructor(decl);

            // public variables: add reference qualifier
            addReferenceQualifierToPublicFields(rewriter, context);
        }

        virtual void addReferenceQualifierToPublicFields(Rewriter& rewriter, ASTContext& context) = 0;

    public:
        
        ClassDefinition(Rewriter& rewriter)
            :
            rewriter(rewriter)
        { ; }
    };

    class CXXClassDefinition : public ClassDefinition
    {
        using baseClass = ClassDefinition;

        using baseClass::rewriter;
        using baseClass::classInfo;
        using baseClass::matchAndModify;

        virtual void addReferenceQualifierToPublicFields(Rewriter& rewriter, ASTContext& context)
        {
            matchAndModify(context, fieldDecl(allOf(isPublic(), hasParent(cxxRecordDecl(hasName(classInfo.className))))).bind("fieldDecl"),
                [] (const MatchFinder::MatchResult& result, Rewriter& rewriter)
                { 
                    if (const FieldDecl* decl = result.Nodes.getNodeAs<FieldDecl>("fieldDecl"))
                    {        
                        rewriter.ReplaceText(decl->getSourceRange(), decl->getType().getAsString() + "& " + decl->getNameAsString());
                    }
                }
            );
        }

    public:
        
        CXXClassDefinition(Rewriter& rewriter)
            :
            baseClass(rewriter)
        { ; }

        virtual void run(const MatchFinder::MatchResult& result)
        {
            if (const CXXRecordDecl* decl = result.Nodes.getNodeAs<CXXRecordDecl>("classDecl"))
            {
                classInfo.isTemplateClass = false;
                classInfo.isTemplateClassDefinition = false;
                collectMetaInformation(*decl, *result.Context);

                if (classInfo.isProxyClassCandidate)
                {      
                    const std::string original_class = dumpDeclToStringHumanReadable(decl, rewriter.getLangOpts(), false);
                    baseClass::generateProxyClass(*decl, *result.Context);
                    rewriter.InsertText(decl->getSourceRange().getBegin(), original_class + ";\n\n", true, true);
                }
            }
        }
    };

    class ClassTemplateDefinition : public ClassDefinition
    {
        using baseClass = ClassDefinition;

        using baseClass::rewriter;
        using baseClass::classInfo;
        using baseClass::matchAndModify;

        virtual void addReferenceQualifierToPublicFields(Rewriter& rewriter, ASTContext& context)
        {
            matchAndModify(context, fieldDecl(allOf(isPublic(), hasParent(cxxRecordDecl(allOf(hasName(classInfo.className), unless(isTemplateInstantiation())))))).bind("fieldDecl"),
                [] (const MatchFinder::MatchResult& result, Rewriter& rewriter)
                { 
                    if (const FieldDecl* decl = result.Nodes.getNodeAs<FieldDecl>("fieldDecl"))
                    {        
                        rewriter.ReplaceText(decl->getSourceRange(), decl->getType().getAsString() + "& " + decl->getNameAsString());
                    }
                }
            );
        }

    public:
        
        ClassTemplateDefinition(Rewriter& rewriter)
            :
            baseClass(rewriter)
        { ; }
        
        virtual void run(const MatchFinder::MatchResult& result)
        {
            if (const ClassTemplateDecl* decl = result.Nodes.getNodeAs<ClassTemplateDecl>("classTemplateDecl"))
            {
                classInfo.isTemplateClass = true;
                classInfo.isTemplateClassDefinition = decl->isThisDeclarationADefinition();
                if (!classInfo.isTemplateClassDefinition) return;
                collectMetaInformation(*(decl->getTemplatedDecl()), *result.Context);

                if (classInfo.isProxyClassCandidate)
                {
                    const std::string original_class = dumpDeclToStringHumanReadable(decl, rewriter.getLangOpts(), false);
                    baseClass::generateProxyClass(*(decl->getTemplatedDecl()), *result.Context);
                    rewriter.InsertText(decl->getSourceRange().getBegin(), original_class + ";\n\n", true, true);
                }
            }
        }
    };

    class FindProxyClassCandidate : public ASTConsumer
    {
        CXXClassDefinition cxxClassHandler;
        ClassTemplateDefinition classTemplateHandler;
        MatchFinder matcher;
        
    public:
        
        FindProxyClassCandidate(Rewriter& rewriter) 
            :
            cxxClassHandler(rewriter),
            classTemplateHandler(rewriter)
        {
            matcher.addMatcher(cxxRecordDecl(allOf(isDefinition(), unless(isTemplateInstantiation()))).bind("classDecl"), &cxxClassHandler);
            matcher.addMatcher(classTemplateDecl().bind("classTemplateDecl"), &classTemplateHandler);
        }

        void HandleTranslationUnit(ASTContext& context) override
        {	
            matcher.matchAST(context);
        }
    };

    class InsertProxyClass : public ASTFrontendAction
    {
        Rewriter rewriter;
        
    public:
        
        InsertProxyClass() { ; }
        
        void EndSourceFileAction() override
        {
            rewriter.getEditBuffer(rewriter.getSourceMgr().getMainFileID()).write(llvm::outs());
        }
        
        std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance& ci, StringRef file) override
        {
            rewriter.setSourceMgr(ci.getSourceManager(), ci.getLangOpts());
            return llvm::make_unique<FindProxyClassCandidate>(rewriter);
        }
    };
}

#endif