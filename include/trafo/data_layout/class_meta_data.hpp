// Copyright (c) 2017-2018 Florian Wende (flwende@gmail.com)
//
// Distributed under the BSD 2-clause Software License
// (See accompanying file LICENSE)

#if !defined(TRAFO_DATA_LAYOUT_CLASS_META_DATA_HPP)
#define TRAFO_DATA_LAYOUT_CLASS_META_DATA_HPP

#include <string>
#include <vector>
#include <cstdint>
#include <misc/matcher.hpp>

#if !defined(TRAFO_NAMESPACE)
    #define TRAFO_NAMESPACE fw
#endif

namespace TRAFO_NAMESPACE
{
    namespace internal
    {
        class ClassMetaData
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
                const std::string name;
                const std::string typeName;
                const bool isPublic;
                const bool isConst;
                const bool isFundamentalOrTemplateType;
                
                MemberField(const clang::FieldDecl& decl, const std::string typeName, const std::string name, const bool isPublic, const bool isConst, const bool isFundamentalOrTemplateType)
                    :
                    decl(decl),
                    name(name),
                    typeName(typeName),
                    isPublic(isPublic),
                    isConst(isConst),
                    isFundamentalOrTemplateType(isFundamentalOrTemplateType)
                { ; }
            };
            
            void collectMetaData()
            {
                Matcher matcher;

                // match all (public, private) fields of the class and collect information: 
                // declaration, const qualifier, fundamental type, type name, name
                std::string publicFieldTypeName;
                for (auto field : decl.fields())
                {
                    const clang::QualType qualType = field->getType();
                    const clang::Type* type = qualType.getTypePtrOrNull();

                    const std::string name = field->getType().getAsString();
                    const std::string typeName = field->getNameAsString();
                    const bool isPublic = Matcher::testDecl(*field, clang::ast_matchers::fieldDecl(clang::ast_matchers::isPublic()), context);
                    const bool isConstant = qualType.getQualifiers().hasConst();
                    const bool isFundamentalOrTemplateType = (type != nullptr ? (type->isFundamentalType() || type->isTemplateTypeParmType()) : false);
                    fields.emplace_back(*field, name, typeName, isPublic, isConstant, isFundamentalOrTemplateType);

                    if (isPublic)
                    {
                        const MemberField& thisField = fields.back();

                        if (numPublicFields == 0)
                        {
                            publicFieldTypeName = thisField.typeName;
                        }
                        else
                        {
                            hasMultiplePublicFieldTypes |= (thisField.typeName != publicFieldTypeName);
                        }

                        hasNonFundamentalPublicFields |= !(thisField.isFundamentalOrTemplateType);

                        ++numPublicFields;
                    }
                }
                
                // proxy class candidates must have at least 1 public field
                isProxyClassCandidate &= (numPublicFields > 0);
                if (!isProxyClassCandidate) return;

                // proxy class candidates must have fundamental public fields
                isProxyClassCandidate &= !hasNonFundamentalPublicFields;

                if (isProxyClassCandidate);
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
            const bool isStruct;
            const bool isTemplated;
            const std::uint32_t numFields;
            std::uint32_t numPublicFields;
            bool hasNonFundamentalPublicFields;
            bool hasMultiplePublicFieldTypes;
            bool isProxyClassCandidate;
            std::vector<AccessSpecifier> publicAccess;
            std::vector<AccessSpecifier> privateAccess;
            std::vector<Constructor> publicConstructors;
            std::vector<Constructor> privateConstructors;
            std::vector<MemberField> fields;

            // proxy class candidates should not have any of these properties: abstract, polymorphic, empty AND
            // proxy class candidates must be class definitions and no specializations in case of template classes
            // note: a template class specialization is of type CXXRecordDecl, and we need to check for its describing template class
            //       (if it does not exist, then it is a normal C++ class)
            ClassMetaData(const clang::CXXRecordDecl& decl, clang::ASTContext& context, const bool isTemplated)
                :
                decl(decl),
                context(context),
                name(decl.getNameAsString()),                
                isStruct(Matcher::testDecl(decl, clang::ast_matchers::recordDecl(clang::ast_matchers::isStruct()), context)),
                isTemplated(isTemplated),
                numFields(std::distance(decl.field_begin(), decl.field_end())),
                numPublicFields(0),
                hasNonFundamentalPublicFields(false),
                hasMultiplePublicFieldTypes(false),
                isProxyClassCandidate(numFields > 0 && (isTemplated ? true : decl.getDescribedClassTemplate() == nullptr) &&
                                        !(decl.isAbstract() || decl.isPolymorphic() || decl.isEmpty()))
            {
                if (isProxyClassCandidate)
                {
                    collectMetaData();
                }
            }

            clang::SourceLocation publicScopeStart()
            {
                if (publicAccess.size() > 0)
                {
                    return publicAccess[0].scopeBegin;
                }
                else if (isStruct)
                {
                    return decl.getBraceRange().getBegin().getLocWithOffset(1);
                }
                else
                {
                    return decl.getBraceRange().getEnd();
                }
            }
        };
    }
}

#endif