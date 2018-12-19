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

#if !defined(TRAFO_NAMESPACE)
    #define TRAFO_NAMESPACE fw
#endif

namespace TRAFO_NAMESPACE
{
    using namespace TRAFO_NAMESPACE::internal;

    class ClassDefinition : public clang::ast_matchers::MatchFinder::MatchCallback
    {
    protected:

        clang::Rewriter& rewriter;

        // generic handler definitions
        class MyRewriter : public clang::ast_matchers::MatchFinder::MatchCallback
        {
            clang::Rewriter& rewriter;
            std::function<void(const clang::ast_matchers::MatchFinder::MatchResult&, clang::Rewriter&)> kernel;

        public:

            template <typename F>
            MyRewriter(clang::Rewriter& rewriter, const F& kernel)
                :
                rewriter(rewriter),
                kernel(kernel)  
            { ; }
            
            virtual void run(const clang::ast_matchers::MatchFinder::MatchResult& result)
            {
                kernel(result, rewriter);
            }

            template <typename T>
            void matchAndModify(clang::ASTContext& context, const T& match)
            {
                clang::ast_matchers::MatchFinder matcher;
                matcher.addMatcher(match, this);
                matcher.matchAST(context);
            }
        };

        template <typename T, typename F>
        void matchAndModify(clang::ASTContext& context, const T& match, const F& kernel)
        {
            MyRewriter myRewriter(rewriter, kernel);
            clang::ast_matchers::MatchFinder matcher;
            matcher.addMatcher(match, &myRewriter);
            matcher.matchAST(context);
        }

        class MyMatchFinder : public clang::ast_matchers::MatchFinder::MatchCallback
        {
            std::function<void(const clang::ast_matchers::MatchFinder::MatchResult&)> kernel;

        public:

            template <typename F>
            MyMatchFinder(const F& kernel)
                :
                kernel(kernel)  
            { ; }
            
            virtual void run(const clang::ast_matchers::MatchFinder::MatchResult& result)
            {
                kernel(result);
            }

            template <typename T>
            void match(clang::ASTContext& context, const T& match)
            {
                clang::ast_matchers::MatchFinder matcher;
                matcher.addMatcher(match, this);
                matcher.matchAST(context);
            }
        };

        template <typename T, typename F>
        static void match(clang::ASTContext& context, const T& match, const F& kernel)
        {
            MyMatchFinder myMatcher(kernel);
            clang::ast_matchers::MatchFinder matcher;
            matcher.addMatcher(match, &myMatcher);
            matcher.matchAST(context);
        }

        // meta data
        class MetaData
        {
            struct AccessSpecifier
            {
                const clang::AccessSpecDecl& decl;
                const clang::SourceLocation sourceLocation;
                const clang::SourceRange sourceRange;
                const clang::SourceLocation scopeBegin;
                AccessSpecifier(const clang::AccessSpecDecl& decl)
                    :
                    decl(decl),
                    sourceLocation(decl.getAccessSpecifierLoc()),
                    sourceRange(decl.getSourceRange()),
                    scopeBegin(decl.getColonLoc().getLocWithOffset(1))
                { ; }
            };

            struct Constructor
            {
                const clang::CXXConstructorDecl& decl;
                Constructor(const clang::CXXConstructorDecl& decl)
                    :
                    decl(decl)
                { ; }
            };

            struct MemberField
            {
                const clang::FieldDecl& decl;
                const bool isConst;
                const bool isFundamentalOrTemplateType;
                const std::string typeName;
                const std::string name;
                MemberField(const clang::FieldDecl& decl, const bool isConst, const bool isFundamentalOrTemplateType, const std::string typeName, const std::string name)
                    :
                    decl(decl),
                    isConst(isConst),
                    isFundamentalOrTemplateType(isFundamentalOrTemplateType),
                    typeName(typeName),
                    name(name)
                { ; }
            };

            void collectMetaData()
            {
                // isTemplateClass has been set already
                // isTemplateClassDefinition has been set already

                // proxy class candidates should not have any of these properties: abstract, polymorphic, empty
                isProxyClassCandidate = !(decl.isAbstract() || decl.isPolymorphic() || decl.isEmpty());
                // proxy class candidates must be class definitions and no specializations in case of template classes
                // note: a template class specialization is of type CXXRecordDecl, and we need to check for its describing template class
                //       (if it does not exist, then it is a normal C++ class)
                if (!isTemplateClass) isProxyClassCandidate &= (decl.getDescribedClassTemplate() == nullptr);
                if (!isProxyClassCandidate) return;
                    
                numFields = std::distance(decl.field_begin(), decl.field_end());
                if (numFields > 0)
                {
                    // match all (public, private) fields of the class and collect information: declaration, const qualifier, fundamental type, type name, name
                    using namespace clang::ast_matchers;
                #define MATCH(MODIFIER, VARIABLE) \
                    ClassDefinition::match(context, fieldDecl(allOf(MODIFIER(), hasParent(cxxRecordDecl(allOf(hasName(name), unless(isTemplateInstantiation())))))).bind("fieldDecl"), \
                        [&] (const MatchFinder::MatchResult& result) mutable \
                        { \
                            if (const clang::FieldDecl* decl = result.Nodes.getNodeAs<clang::FieldDecl>("fieldDecl")) \
                            { \
                                const clang::QualType qualType = decl->getType(); \
                                const bool isConstant = qualType.getQualifiers().hasConst(); \
                                const clang::Type* type = qualType.getTypePtrOrNull(); \
                                const bool isFundamentalOrTemplateType = (type != nullptr ? (type->isFundamentalType() || type->isTemplateTypeParmType()) : false); \
                                VARIABLE.emplace_back(*decl, isConstant, isFundamentalOrTemplateType, decl->getType().getAsString(), decl->getNameAsString()); \
                            } \
                        })
                    
                    MATCH(isPublic, publicFields);
                    isProxyClassCandidate &= (publicFields.size() > 0);
                    // proxy class candidates must have at least 1 public field
                    if (!isProxyClassCandidate) return;
                    MATCH(isPrivate, privateFields);
                #undef MATCH
                }

                hasNonFundamentalPublicFields = false;
                hasMultiplePublicFieldTypes = false;
                const std::string typeName = publicFields[0].typeName;
                for (auto field : publicFields)
                {
                    hasNonFundamentalPublicFields |= !(field.isFundamentalOrTemplateType);
                    hasMultiplePublicFieldTypes |= (field.typeName != typeName);
                }
                // proxy class candidates must have fundamental public fields
                isProxyClassCandidate &= !hasNonFundamentalPublicFields;

                // everything below is executed only of this class is a proxy class candidate
                if (isProxyClassCandidate)
                {
                    using namespace clang::ast_matchers;
                    // match public and private access specifier
                    ClassDefinition::match(context, accessSpecDecl(hasParent(cxxRecordDecl(allOf(hasName(name), unless(isTemplateInstantiation()))))).bind("accessSpecDecl"), \
                        [&] (const MatchFinder::MatchResult& result) mutable \
                        {
                            if (const clang::AccessSpecDecl* decl = result.Nodes.getNodeAs<clang::AccessSpecDecl>("accessSpecDecl"))
                            {
                                publicAccess.emplace_back(*decl);
                            }
                        });
                    
                    // match all (public, private) constructors
                #define MATCH(MODIFIER, VARIABLE) \
                    ClassDefinition::match(context, cxxConstructorDecl(allOf(MODIFIER(), hasParent(cxxRecordDecl(allOf(hasName(name), unless(isTemplateInstantiation())))))).bind("constructorDecl"), \
                        [&] (const MatchFinder::MatchResult& result) mutable \
                        { \
                            if (const clang::CXXConstructorDecl* decl = result.Nodes.getNodeAs<clang::CXXConstructorDecl>("constructorDecl")) \
                            { \
                                VARIABLE.emplace_back(*decl); \
                            } \
                        })
                        
                    MATCH(isPublic, publicConstructors);
                    MATCH(isPrivate, privateConstructors);
                #undef MATCH
                }
            }

        public:

            const clang::CXXRecordDecl& decl;
            clang::ASTContext& context;
            const std::string name;
            bool isTemplateClass;
            std::vector<AccessSpecifier> publicAccess;
            std::vector<AccessSpecifier> privateAccess;
            std::vector<Constructor> publicConstructors;
            std::vector<Constructor> privateConstructors;
            std::vector<MemberField> publicFields;
            std::vector<MemberField> privateFields;
            std::size_t numFields;
            bool hasNonFundamentalPublicFields;
            bool hasMultiplePublicFieldTypes;
            bool isProxyClassCandidate;

            MetaData(const clang::CXXRecordDecl& decl, clang::ASTContext& context, const bool isTemplateClass)
                :
                decl(decl),
                context(context),
                name(decl.getNameAsString()),                
                isTemplateClass(isTemplateClass)
            {
                collectMetaData();
            }
        };

        std::vector<MetaData> targetClasses;

        // implementation

        std::string createProxyClassStandardConstructor(MetaData& thisClass, const clang::CXXRecordDecl& decl) const
        {
            // CONTINUE HERE
            std::string constructor = thisClass.name + "_proxy(";

            if (thisClass.hasMultiplePublicFieldTypes)
            {
                std::cerr << thisClass.name << ": createProxyClassStandardConstructor: not implemented yet" << std::endl;
            }
            else
            {
                constructor += (thisClass.publicFields[0].isConst ? "const " : "");
                constructor += thisClass.publicFields[0].typeName + "* ptr, const std::size_t n)\n\t:";
            }

            return constructor;
        }

        // generate proxy class definition
        void generateProxyClass(MetaData& thisClass)
        {
            const clang::CXXRecordDecl& decl = thisClass.decl;
            clang::ASTContext& context = thisClass.context;

            // replace class name
            rewriter.ReplaceText(decl.getLocation(), thisClass.name + "_proxy");

            // remove original constructors
            for (auto ctor : decl.ctors())
            {
                rewriter.ReplaceText(ctor->getSourceRange(), "// constructor: removed");    
            }

            // insert standard constructor
            const std::string constructor = createProxyClassStandardConstructor(thisClass, decl) + "\n";
            if (thisClass.publicConstructors.size() > 0)
            {
                rewriter.ReplaceText(thisClass.publicConstructors[0].decl.getSourceRange(), constructor);
            }
            else
            {
                const clang::SourceLocation location = (thisClass.publicAccess.size() > 0 ? thisClass.publicAccess[0].scopeBegin : decl.getBraceRange().getEnd());
                const std::string prefix = (thisClass.publicAccess.size() > 0 ? "\n\t" : "public:\n\t");
                rewriter.InsertText(location, prefix + constructor, true, true);
            }

            // public variables: add reference qualifier
            addReferenceQualifierToPublicFields(thisClass, rewriter);
        }

        virtual void addReferenceQualifierToPublicFields(MetaData& , clang::Rewriter& rewriter) = 0;

    public:
        
        ClassDefinition(clang::Rewriter& rewriter)
            :
            rewriter(rewriter)
        { ; }
    };

    class CXXClassDefinition : public ClassDefinition
    {
        using baseClass = ClassDefinition;

        using baseClass::rewriter;
        using baseClass::targetClasses;
        using baseClass::matchAndModify;

        virtual void addReferenceQualifierToPublicFields(MetaData& thisClass, clang::Rewriter& rewriter)
        {
            using namespace clang::ast_matchers;
            clang::ASTContext& context = thisClass.context;
            matchAndModify(context, fieldDecl(allOf(isPublic(), hasParent(cxxRecordDecl(hasName(thisClass.name))))).bind("fieldDecl"),
                [] (const MatchFinder::MatchResult& result, clang::Rewriter& rewriter)
                { 
                    if (const clang::FieldDecl* decl = result.Nodes.getNodeAs<clang::FieldDecl>("fieldDecl"))
                    {        
                        rewriter.ReplaceText(decl->getSourceRange(), decl->getType().getAsString() + "& " + decl->getNameAsString());
                    }
                }
            );
        }

    public:
        
        CXXClassDefinition(clang::Rewriter& rewriter)
            :
            baseClass(rewriter)
        { ; }

        virtual void run(const clang::ast_matchers::MatchFinder::MatchResult& result)
        {
            if (const clang::CXXRecordDecl* decl = result.Nodes.getNodeAs<clang::CXXRecordDecl>("classDecl"))
            {
                targetClasses.emplace_back(*decl, *result.Context, false);
                MetaData& thisClass = targetClasses.back();
                //collectMetaInformation(thisClass);

                if (thisClass.isProxyClassCandidate)
                {      
                    const std::string original_class = dumpDeclToStringHumanReadable(decl, rewriter.getLangOpts(), false);
                    baseClass::generateProxyClass(thisClass);
                    rewriter.InsertText(decl->getSourceRange().getBegin(), original_class + ";\n\n", true, true);
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
        using baseClass = ClassDefinition;

        using baseClass::rewriter;
        using baseClass::targetClasses;
        using baseClass::matchAndModify;

        virtual void addReferenceQualifierToPublicFields(MetaData& thisClass, clang::Rewriter& rewriter)
        {
            using namespace clang::ast_matchers;
            clang::ASTContext& context = thisClass.context;
            matchAndModify(context, fieldDecl(allOf(isPublic(), hasParent(cxxRecordDecl(allOf(hasName(thisClass.name), unless(isTemplateInstantiation())))))).bind("fieldDecl"),
                [] (const MatchFinder::MatchResult& result, clang::Rewriter& rewriter)
                { 
                    if (const clang::FieldDecl* decl = result.Nodes.getNodeAs<clang::FieldDecl>("fieldDecl"))
                    {        
                        rewriter.ReplaceText(decl->getSourceRange(), decl->getType().getAsString() + "& " + decl->getNameAsString());
                    }
                }
            );
        }

    public:
        
        ClassTemplateDefinition(clang::Rewriter& rewriter)
            :
            baseClass(rewriter)
        { ; }
        
        virtual void run(const clang::ast_matchers::MatchFinder::MatchResult& result)
        {
            if (const clang::ClassTemplateDecl* decl = result.Nodes.getNodeAs<clang::ClassTemplateDecl>("classTemplateDecl"))
            {
                const bool isTemplateClassDefinition = decl->isThisDeclarationADefinition();
                if (!isTemplateClassDefinition) return;

                targetClasses.emplace_back(*(decl->getTemplatedDecl()), *result.Context, true);
                MetaData& thisClass = targetClasses.back();
                //collectMetaInformation(thisClass);

                if (thisClass.isProxyClassCandidate)
                {
                    const std::string original_class = dumpDeclToStringHumanReadable(decl, rewriter.getLangOpts(), false);
                    baseClass::generateProxyClass(thisClass);
                    rewriter.InsertText(decl->getSourceRange().getBegin(), original_class + ";\n\n", true, true);
                }
                else
                {
                    targetClasses.pop_back();
                }
            }
        }
    };

    class FindProxyClassCandidate : public clang::ASTConsumer
    {
        CXXClassDefinition cxxClassHandler;
        ClassTemplateDefinition classTemplateHandler;
        clang::ast_matchers::MatchFinder matcher;
        
    public:
        
        FindProxyClassCandidate(clang::Rewriter& rewriter) 
            :
            cxxClassHandler(rewriter),
            classTemplateHandler(rewriter)
        {
            using namespace clang::ast_matchers;

            matcher.addMatcher(cxxRecordDecl(allOf(isDefinition(), unless(isTemplateInstantiation()))).bind("classDecl"), &cxxClassHandler);
            matcher.addMatcher(classTemplateDecl().bind("classTemplateDecl"), &classTemplateHandler);
        }

        void HandleTranslationUnit(clang::ASTContext& context) override
        {	
            matcher.matchAST(context);
        }
    };

    class InsertProxyClass : public clang::ASTFrontendAction
    {
        clang::Rewriter rewriter;
        
    public:
        
        InsertProxyClass() { ; }
        
        void EndSourceFileAction() override
        {
            rewriter.getEditBuffer(rewriter.getSourceMgr().getMainFileID()).write(llvm::outs());
        }
        
        std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance& ci, llvm::StringRef file) override
        {
            rewriter.setSourceMgr(ci.getSourceManager(), ci.getLangOpts());
            return llvm::make_unique<FindProxyClassCandidate>(rewriter);
        }
    };
}

#endif