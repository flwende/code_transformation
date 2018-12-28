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
#include <misc/matcher.hpp>

#if !defined(TRAFO_NAMESPACE)
    #define TRAFO_NAMESPACE fw
#endif

namespace TRAFO_NAMESPACE
{
    using namespace TRAFO_NAMESPACE::internal;

    class ClassDefinition : public clang::ast_matchers::MatchFinder::MatchCallback
    {
    protected:

        Rewriter& rewriter;

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
                Matcher matcher;

                if (numFields > 0)
                {
                    using namespace clang::ast_matchers;

                    // match all (public, private) fields of the class and collect information: 
                    // declaration, const qualifier, fundamental type, type name, name
                #define MATCH(MODIFIER, VARIABLE) \
                    matcher.addMatcher(fieldDecl(allOf(MODIFIER(), hasParent(cxxRecordDecl(allOf(hasName(name), unless(isTemplateInstantiation())))))).bind("fieldDecl"), \
                        [&] (const MatchFinder::MatchResult& result) mutable \
                        { \
                            if (!isProxyClassCandidate) return; \
                            \
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
                    MATCH(isPrivate, privateFields);
                #undef MATCH

                    matcher.run(context);
                    matcher.clear();
                }
                
                isProxyClassCandidate &= (publicFields.size() > 0);
                // proxy class candidates must have at least 1 public field
                if (!isProxyClassCandidate) return;

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
                    matcher.addMatcher(accessSpecDecl(hasParent(cxxRecordDecl(allOf(hasName(name), unless(isTemplateInstantiation()))))).bind("accessSpecDecl"), \
                        [&] (const MatchFinder::MatchResult& result) mutable \
                        {
                            if (const clang::AccessSpecDecl* decl = result.Nodes.getNodeAs<clang::AccessSpecDecl>("accessSpecDecl"))
                            {
                                publicAccess.emplace_back(*decl);
                            }
                        });
                    
                    // match all (public, private) constructors
                #define MATCH(MODIFIER, VARIABLE) \
                    matcher.addMatcher(cxxConstructorDecl(allOf(MODIFIER(), hasParent(cxxRecordDecl(allOf(hasName(name), unless(isTemplateInstantiation())))))).bind("constructorDecl"), \
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

                    matcher.run(context);
                    matcher.clear();
                }
            }

        public:

            const clang::CXXRecordDecl& decl;
            clang::ASTContext& context;
            const std::string name;
            std::size_t numFields;
            bool isTemplateClass;
            bool hasNonFundamentalPublicFields;
            bool hasMultiplePublicFieldTypes;
            bool isProxyClassCandidate;
            std::vector<AccessSpecifier> publicAccess;
            std::vector<AccessSpecifier> privateAccess;
            std::vector<Constructor> publicConstructors;
            std::vector<Constructor> privateConstructors;
            std::vector<MemberField> publicFields;
            std::vector<MemberField> privateFields;

            MetaData(const clang::CXXRecordDecl& decl, clang::ASTContext& context, const bool isTemplateClass)
                :
                decl(decl),
                context(context),
                name(decl.getNameAsString()),                
                numFields(std::distance(decl.field_begin(), decl.field_end())),
                isTemplateClass(isTemplateClass),
                hasNonFundamentalPublicFields(false),
                hasMultiplePublicFieldTypes(false),
                // proxy class candidates should not have any of these properties: abstract, polymorphic, empty AND
                // proxy class candidates must be class definitions and no specializations in case of template classes
                // note: a template class specialization is of type CXXRecordDecl, and we need to check for its describing template class
                //       (if it does not exist, then it is a normal C++ class)
                isProxyClassCandidate((isTemplateClass ? true : decl.getDescribedClassTemplate() == nullptr) &&
                                      !(decl.isAbstract() || decl.isPolymorphic() || decl.isEmpty()))
            {
                if (isProxyClassCandidate)
                {
                    collectMetaData();
                }
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
            rewriter.replace(decl.getLocation(), thisClass.name + "_proxy");

            // remove original constructors
            for (auto ctor : decl.ctors())
            {
                rewriter.replace(ctor->getSourceRange(), "// constructor: removed");    
            }

            // insert standard constructor
            const std::string constructor = createProxyClassStandardConstructor(thisClass, decl) + "\n";
            if (thisClass.publicConstructors.size() > 0)
            {
                rewriter.replace(thisClass.publicConstructors[0].decl.getSourceRange(), constructor);
            }
            else
            {
                const clang::SourceLocation location = (thisClass.publicAccess.size() > 0 ? thisClass.publicAccess[0].scopeBegin : decl.getBraceRange().getEnd());
                const std::string prefix = (thisClass.publicAccess.size() > 0 ? "\n\t" : "public:\n\t");
                rewriter.insert(location, prefix + constructor, true, true);
            }

            // public variables: add reference qualifier
            addReferenceQualifierToPublicFields(thisClass, rewriter);
        }

        virtual void addReferenceQualifierToPublicFields(MetaData& , Rewriter& rewriter) = 0;

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

        virtual void addReferenceQualifierToPublicFields(MetaData& thisClass, Rewriter& rewriter)
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
                MetaData& thisClass = targetClasses.back();
                //collectMetaInformation(thisClass);

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

        virtual void addReferenceQualifierToPublicFields(MetaData& thisClass, Rewriter& rewriter)
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
                const bool isTemplateClassDefinition = decl->isThisDeclarationADefinition();
                if (!isTemplateClassDefinition) return;

                targetClasses.emplace_back(*(decl->getTemplatedDecl()), *result.Context, true);
                MetaData& thisClass = targetClasses.back();
                //collectMetaInformation(thisClass);

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

    class FindProxyClassCandidate : public clang::ASTConsumer
    {
        Rewriter rewriter;
        CXXClassDefinition cxxClassHandler;
        ClassTemplateDefinition classTemplateHandler;
        clang::ast_matchers::MatchFinder matcher;
        
    public:
        
        FindProxyClassCandidate(clang::Rewriter& clangRewriter) 
            :
            rewriter(clangRewriter),
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