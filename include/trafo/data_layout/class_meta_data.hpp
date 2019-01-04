// Copyright (c) 2017-2018 Florian Wende (flwende@gmail.com)
//
// Distributed under the BSD 2-clause Software License
// (See accompanying file LICENSE)

#if !defined(TRAFO_DATA_LAYOUT_CLASS_META_DATA_HPP)
#define TRAFO_DATA_LAYOUT_CLASS_META_DATA_HPP

#include <string>
#include <vector>
#include <cstdint>
#include <type_traits>
#include <misc/ast_helper.hpp>
#include <misc/matcher.hpp>
#include <misc/string_helper.hpp>

#if !defined(TRAFO_NAMESPACE)
    #define TRAFO_NAMESPACE fw
#endif

namespace TRAFO_NAMESPACE
{
    namespace internal
    {
#if defined(old)
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
            ClassMetaData(const clang::CXXRecordDecl& decl, clang::ASTContext& context)
                :
                decl(decl),
                context(context),
                name(decl.getNameAsString()),                
                isStruct(Matcher::testDecl(decl, clang::ast_matchers::recordDecl(clang::ast_matchers::isStruct()), context)),
                numFields(std::distance(decl.field_begin(), decl.field_end())),
                numPublicFields(0),
                hasNonFundamentalPublicFields(false),
                hasMultiplePublicFieldTypes(false),
                isProxyClassCandidate(numFields > 0 && (decl.getDescribedClassTemplate() == nullptr) &&
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
#else


        class ClassMetaData
        {
            class AccessSpecifier
            {
            public:

                enum class Type {Public = 0, Protected = 1, Private = 2};

                const clang::AccessSpecDecl& decl;
                const clang::SourceRange sourceRange;
                const clang::SourceLocation scopeBegin;
                const Type type;
                const std::string name;

                AccessSpecifier(const clang::AccessSpecDecl& decl, Type type)
                    :
                    decl(decl),
                    sourceRange(decl.getSourceRange()),
                    scopeBegin(decl.getColonLoc().getLocWithOffset(1)),
                    type(type),
                    name(type == Type::Public ? "public" : (type == Type::Protected ? "protected" : "private"))
                { ; }

                void printInfo(clang::SourceManager& sm, const std::string indent = "") const
                {
                    std::cout << indent << "* " << name << std::endl;
                    std::cout << indent << "\t+-> range: " << sourceRange.printToString(sm) << std::endl;
                    std::cout << indent << "\t+-> scope begin: " << scopeBegin.printToString(sm) << std::endl;
                }
            };

            class Field
            {
            public:
                            
                const clang::FieldDecl& decl;
                const clang::SourceRange sourceRange;
                const std::string name;
                const clang::Type* type;
                const std::string typeName;
                const AccessSpecifier::Type access;
                const bool isPublic;
                const bool isProtected;
                const bool isPrivate;
                const bool isConst;
                const bool isFundamentalOrTemplated;
                
                Field(const clang::FieldDecl& decl, const AccessSpecifier::Type access)
                    :
                    decl(decl),
                    sourceRange(decl.getSourceRange()),
                    name(decl.getNameAsString()),
                    type(decl.getType().getTypePtrOrNull()),
                    typeName(decl.getType().getAsString()),
                    access(access),
                    isPublic(access == AccessSpecifier::Type::Public),
                    isProtected(access == AccessSpecifier::Type::Protected),
                    isPrivate(access == AccessSpecifier::Type::Private),
                    isConst(decl.getType().isConstQualified()),
                    isFundamentalOrTemplated(type != nullptr ? (type->isFundamentalType() || type->isTemplateTypeParmType()) : false)
                { ; }

                void printInfo(clang::SourceManager& sm, const std::string indent = "") const
                {
                    std::cout << indent << "* name=" << name << ", type=" << typeName << (isConst ? " (const qualified)" : "") << std::endl;
                    std::cout << indent << "\t+-> range: " << sourceRange.printToString(sm) << std::endl;
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
                const bool hasBody;
                const AccessSpecifier::Type access;
                const bool isPublic;
                const bool isProtected;
                const bool isPrivate;

                Constructor(const clang::CXXConstructorDecl& decl, const AccessSpecifier::Type access)
                    :
                    decl(decl),
                    sourceRange(decl.getSourceRange()),
                    isDefaultConstructor(decl.isDefaultConstructor()),
                    isCopyConstructor(decl.isCopyConstructor()),
                    hasBody(decl.hasBody()),
                    access(access),
                    isPublic(access == AccessSpecifier::Type::Public),
                    isProtected(access == AccessSpecifier::Type::Protected),
                    isPrivate(access == AccessSpecifier::Type::Private)
                { ; }

                void printInfo(clang::SourceManager& sm, const std::string indent = "") const
                {
                    std::cout << indent << "* " << (isDefaultConstructor ? "default " : (isCopyConstructor ? "copy " : "")) << "constructor" << std::endl;
                    std::cout << indent << "\t+-> range: " << sourceRange.printToString(sm) << std::endl;
                    std::cout << indent << "\t+-> access: " << (isPublic ? "public" : (isProtected ? "protected" : "private")) << std::endl;
                    std::cout << indent << "\t+-> has body: " << (hasBody ? "yes" : "no") << std::endl;
                }
            };

            class Namespace
            {
            public:
            
                const clang::NamespaceDecl& decl;
                const clang::SourceRange sourceRange;
                const std::string name;

                Namespace(const clang::NamespaceDecl& decl)
                    :
                    decl(decl),
                    sourceRange(decl.getSourceRange()),
                    name(decl.getNameAsString())
                { ; }

                static std::vector<Namespace> getFromDecl(const clang::NamedDecl& decl)
                {
                    using namespace clang::ast_matchers;

                    std::vector<Namespace> namespaces;

                    Matcher matcher;
                    matcher.addMatcher(namespaceDecl(hasDescendant(namedDecl(hasName(decl.getNameAsString())))).bind("namespaceDecl"),
                        [&] (const MatchFinder::MatchResult& result) mutable
                        {
                            if (const clang::NamespaceDecl* decl = result.Nodes.getNodeAs<clang::NamespaceDecl>("namespaceDecl"))
                            {
                                namespaces.emplace_back(*decl);
                            }
                        });
                    matcher.run(decl.getASTContext());

                    return namespaces;
                }

                void printInfo(clang::SourceManager& sm, const std::string indent = "") const
                {
                    std::cout << indent << "* " << name << ", " << sourceRange.printToString(sm) << std::endl;
                }
            };

            template <typename T>
            class Declaration
            {
                const clang::CXXRecordDecl* cxxRecordDecl;

            public:
            
                const T& decl;
                clang::SourceRange sourceRange;
                std::vector<Namespace> namespaces;
                const bool isDefinition;
                const bool isStruct;
                const bool isClass;

                Declaration(const T& decl, const bool isDefinition)
                    :
                    cxxRecordDecl(ClassDecl::getTemplatedDecl(decl)), // is never nullptr
                    decl(decl),
                    sourceRange(decl.getSourceRange()),
                    namespaces(Namespace::getFromDecl(decl)),
                    isDefinition(isDefinition),
                    isStruct(cxxRecordDecl->isStruct()),
                    isClass(cxxRecordDecl->isClass())
                { ; }

                void printInfo(clang::SourceManager& sm, const std::string indent = "") const
                {
                    std::cout << indent << "* DECLARATION:" << std::endl;
                    std::cout << indent << "\t+-> range: " << sourceRange.printToString(sm) << std::endl;
                    std::cout << indent << "\t+-> namespace(s):" << std::endl;
                    for (auto ns : namespaces)
                    {
                        ns.printInfo(sm, indent + "\t|\t");
                    }
                    std::cout << indent << "\t+-> is definition: " << (isDefinition ? "yes" : "no") << std::endl;
                }
            };

            template <typename T>
            class Definition
            {
                const clang::ClassTemplateDecl* classTemplateDecl;

            public:
            
                const Declaration<T>& declaration;
                const clang::CXXRecordDecl& decl;
                const clang::SourceRange sourceRange;
                const bool isTemplatePartialSpecialization;
                std::vector<Field> fields;
                std::uint32_t numPublicFields;
                std::uint32_t numProtectedFields;
                std::uint32_t numPrivateFields;
                std::vector<AccessSpecifier> accessSpecifiers;
                std::vector<Constructor> constructors;
                

                Definition(const Declaration<T>& declaration, const clang::CXXRecordDecl& decl, const bool isTemplatePartialSpecialization = false)
                    :
                    classTemplateDecl(ClassDecl::getDescribedClassTemplate(decl)), // can be nullptr
                    declaration(declaration),
                    decl(decl),
                    sourceRange(classTemplateDecl ? classTemplateDecl->getSourceRange() : decl.getSourceRange()),
                    isTemplatePartialSpecialization(isTemplatePartialSpecialization),
                    numPublicFields(0),
                    numProtectedFields(0),
                    numPrivateFields(0)
                {
                    using namespace clang::ast_matchers;

                    Matcher matcher;
                    
                    // get fields
                #define ADD_MATCHER(ACCESS) \
                    matcher.addMatcher(fieldDecl(is ## ACCESS()).bind("fieldDecl"), \
                        [&] (const MatchFinder::MatchResult& result) mutable \
                        { \
                            if (const clang::FieldDecl* decl = result.Nodes.getNodeAs<clang::FieldDecl>("fieldDecl")) \
                            { \
                                fields.emplace_back(*decl, AccessSpecifier::Type::ACCESS); \
                                ++num ## ACCESS ## Fields; \
                            } \
                        }, &decl)

                    ADD_MATCHER(Public);
                    ADD_MATCHER(Protected);
                    ADD_MATCHER(Private);
                #undef ADD_MATCHER

                    // get access specifier
                #define ADD_MATCHER(ACCESS) \
                    matcher.addMatcher(accessSpecDecl(is##ACCESS()).bind("accessSpecDecl"), \
                        [&] (const MatchFinder::MatchResult& result) mutable \
                        { \
                            if (const clang::AccessSpecDecl* decl = result.Nodes.getNodeAs<clang::AccessSpecDecl>("accessSpecDecl")) \
                            { \
                                accessSpecifiers.emplace_back(*decl, AccessSpecifier::Type::ACCESS); \
                            } \
                        }, &decl)

                    ADD_MATCHER(Public);
                    ADD_MATCHER(Protected);
                    ADD_MATCHER(Private);
                #undef ADD_MATCHER

                // get constructors
                #define ADD_MATCHER(ACCESS) \
                    matcher.addMatcher(cxxConstructorDecl(is ## ACCESS()).bind("constructorDecl"), \
                        [&] (const MatchFinder::MatchResult& result) mutable \
                        { \
                            if (const clang::CXXConstructorDecl* decl = result.Nodes.getNodeAs<clang::CXXConstructorDecl>("constructorDecl")) \
                            { \
                                constructors.emplace_back(*decl, AccessSpecifier::Type::ACCESS); \
                            } \
                        }, &decl)

                    ADD_MATCHER(Public);
                    ADD_MATCHER(Protected);
                    ADD_MATCHER(Private);
                #undef ADD_MATCHER
                    
                    matcher.run(decl.getASTContext());
                }

                void printInfo(clang::SourceManager& sm, const std::string indent = "") const
                {
                    std::cout << indent << "* DEFINITION:" << std::endl;
                    std::cout << indent << "\t+-> range: " << sourceRange.printToString(sm) << std::endl;
                    std::cout << indent << "\t+-> declaration: " << declaration.sourceRange.printToString(sm) << std::endl;
                    std::cout << indent << "\t+-> is template (partial) specialization: " << (isTemplatePartialSpecialization ? "yes" : "no") << std::endl;
                    std::cout << indent << "\t+-> fields: (public/protected/private)=(" << numPublicFields << "/" << numProtectedFields << "/" << numPrivateFields << ")" << std::endl;
                    for (auto field : fields)
                    {
                        field.printInfo(sm, indent + "\t|\t");
                    }
                    std::cout << indent << "\t+-> access specifier:" << std::endl;
                    for (auto specifier : accessSpecifiers)
                    {
                        specifier.printInfo(sm, indent + "\t|\t");
                    }
                    std::cout << indent << "\t+-> constructor:" << std::endl;
                    for (auto constructor : constructors)
                    {
                        constructor.printInfo(sm, indent + "\t\t");
                    }
                }
            };
            
        public:

            const std::string name;
            const bool isTemplated;
            
            ClassMetaData(const std::string name, const bool isTemplated)
                :
                name(name),
                isTemplated(isTemplated)
            { ; }

            virtual bool addDefinition(const clang::CXXRecordDecl& decl, const bool isTemplatePartialSpecialization = false) = 0;

            virtual void printInfo(clang::SourceManager& sm, const std::string indent = "") const = 0;
        };

        class CXXClassMetaData : public ClassMetaData
        {
            using Base = ClassMetaData;

        public:

            using Base::name;
            const Base::Declaration<clang::CXXRecordDecl> declaration;
            std::vector<Base::Definition<clang::CXXRecordDecl>> definitions;

            CXXClassMetaData(const clang::CXXRecordDecl& decl, const bool isDefinition)
                :
                Base(decl.getNameAsString(), false),
                declaration(decl, isDefinition)
            {
                ;
            }

            virtual bool addDefinition(const clang::CXXRecordDecl& decl, const bool isTemplatePartialSpecialization = false)
            {
                const std::string className = decl.getNameAsString();
                if (className != name) return false;

                for (auto definition : definitions)
                {
                    // already registered
                    if (decl.getSourceRange() == definition.sourceRange) return true;
                }

                definitions.emplace_back(declaration, decl, isTemplatePartialSpecialization);
                return true;
            }

            virtual void printInfo(clang::SourceManager& sm, const std::string indent = "") const
            {
                std::cout << indent << "C++ " << (declaration.isClass ? "class: " : "struct: ") << name << std::endl;
                declaration.printInfo(sm, indent + "\t");
                for (auto definition : definitions)
                {
                    definition.printInfo(sm, indent + "\t");
                }
            }
        };

        class TemplateClassMetaData : public ClassMetaData
        {
            using Base = ClassMetaData;

        public:

            using Base::name;
            const Base::Declaration<clang::ClassTemplateDecl> declaration;
            std::vector<Base::Definition<clang::ClassTemplateDecl>> definitions;

            TemplateClassMetaData(const clang::ClassTemplateDecl& decl, const bool isDefinition)
                :
                Base(decl.getNameAsString(), true),
                declaration(decl, isDefinition)
            {
                ;
            }

            virtual bool addDefinition(const clang::CXXRecordDecl& decl, const bool isTemplatePartialSpecialization = false)
            {
                const std::string className = decl.getNameAsString();
                if (className != name) return false;

                clang::ClassTemplateDecl* classTemplateDecl = decl.getDescribedClassTemplate();
                clang::SourceRange sourceRange = (classTemplateDecl != nullptr ? classTemplateDecl->getSourceRange() : decl.getSourceRange());

                for (auto definition : definitions)
                {
                    // already registered
                    if (sourceRange == definition.sourceRange) return true;
                }

                definitions.emplace_back(declaration, decl, isTemplatePartialSpecialization);
                return true;
            }

            virtual void printInfo(clang::SourceManager& sm, const std::string indent = "") const
            {
                std::cout << indent << "C++ template " << (declaration.isClass ? "class: " : "struct: ") << name << std::endl;
                declaration.printInfo(sm, indent + "\t");
                for (auto definition : definitions)
                {
                    definition.printInfo(sm, indent + "\t");
                }
            }
        };

#if defined(old)
        class CXXClassMetaData : public ClassMetaData
        {
            using Base = ClassMetaData;

            struct AccessSpecifier
            {
                enum class Type {Public = 0, Protected = 1, Private = 2};

                static std::string getAsString(const Type type)
                {
                    switch(type)
                    {
                        case Type::Public : return "public"; break;
                        case Type::Protected : return "protected"; break;
                        case Type::Private: return "private"; break;
                        default: return "";
                    }
                }

                const clang::AccessSpecDecl& decl;
                const clang::SourceRange sourceRange;
                const clang::SourceLocation scopeBegin;
                const AccessSpecifier::Type type;
                const std::string typeName;

                AccessSpecifier(const clang::AccessSpecDecl& decl, Type type)
                    :
                    decl(decl),
                    sourceRange(decl.getSourceRange()),
                    scopeBegin(decl.getColonLoc().getLocWithOffset(1)),
                    type(type),
                    typeName(getAsString(type))
                { ; }

                void printInfo(clang::SourceManager& sm, const std::string indent = "") const
                {
                    std::cout << indent << "access specifier: " << typeName << std::endl;
                    std::cout << indent << "\tdeclaration: " << sourceRange.printToString(sm) << std::endl;
                    std::cout << indent << "\tscope starts at: " << scopeBegin.printToString(sm) << std::endl;            
                }
            };

            struct Constructor
            {
                const clang::CXXConstructorDecl& decl;
                const clang::SourceRange sourceRange;
                const bool hasBody;

                Constructor(const clang::CXXConstructorDecl& decl)
                    :
                    decl(decl),
                    sourceRange(decl.getSourceRange()),
                    hasBody(decl.hasBody())
                { ; }

                void printInfo(clang::SourceManager& sm, const std::string indent = "") const
                {
                    std::cout << indent << "constructor: name=" << decl.getNameAsString() << std::endl;
                    std::cout << indent << "\tdeclaration: " << sourceRange.printToString(sm) << std::endl;
                    std::cout << indent << "\thas body: " << (hasBody ? "yes" : "no") << std::endl;            
                }
            };

            struct MemberField
            {
                const clang::FieldDecl& decl;
                const clang::SourceRange sourceRange;
                const std::string name;
                std::string typeName;
                const AccessSpecifier::Type accessType;
                const bool isPublic;
                const bool isProtected;
                const bool isPrivate;
                const bool isConst;
                const bool isFundamentalOrTemplateType;
                
                //MemberField(const clang::FieldDecl& decl, const std::string typeName, const std::string name, const bool isPublic, const bool isConst, const bool isFundamentalOrTemplateType)
                MemberField(const clang::FieldDecl& decl, const std::string typeName, const std::string name, const AccessSpecifier::Type accessType, const bool isConst, const bool isFundamentalOrTemplateType)
                    :
                    decl(decl),
                    sourceRange(decl.getSourceRange()),
                    name(name),
                    typeName(typeName),
                    accessType(accessType),
                    isPublic(accessType == AccessSpecifier::Type::Public),
                    isProtected(accessType == AccessSpecifier::Type::Protected),
                    isPrivate(accessType == AccessSpecifier::Type::Private),
                    isConst(isConst),
                    isFundamentalOrTemplateType(isFundamentalOrTemplateType)
                { ; }

                void printInfo(clang::SourceManager& sm, const std::string indent = "") const
                {
                    std::cout << indent << "field: name=" << name << ", type=" << typeName << std::endl;
                    std::cout << indent << "\tdeclaration: " << sourceRange.printToString(sm) << std::endl;
                    std::cout << indent << "\tpublic: " << (isPublic ? "yes" : "no") << std::endl;
                    std::cout << indent << "\tconst: " << (isConst ? "yes" : "no") << std::endl;
                    std::cout << indent << "\tfundamental/templated: " << (isFundamentalOrTemplateType ? "yes" : "no") << std::endl;
                }
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

                    const std::string name = field->getNameAsString();
                    const std::string typeName = field->getType().getAsString();
                    const bool isPublic = Matcher::testDecl(*field, clang::ast_matchers::fieldDecl(clang::ast_matchers::isPublic()), context);
                    const bool isProtected = Matcher::testDecl(*field, clang::ast_matchers::fieldDecl(clang::ast_matchers::isProtected()), context);
                    const AccessSpecifier::Type accessType = (isPublic ? AccessSpecifier::Type::Public : (isProtected ? AccessSpecifier::Type::Protected : AccessSpecifier::Type::Private));
                    const bool isConstant = qualType.getQualifiers().hasConst();
                    const bool isFundamentalOrTemplateType = (type != nullptr ? (type->isFundamentalType() || type->isTemplateTypeParmType()) : false);
                    fields.emplace_back(*field, typeName, name, accessType, isConstant, isFundamentalOrTemplateType);

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
                    //matcher.addMatcher(accessSpecDecl(hasParent(cxxRecordDecl(allOf(hasName(name), unless(isTemplateInstantiation()))))).bind("accessSpecDecl"),
                    matcher.addMatcher(accessSpecDecl(hasParent(cxxRecordDecl(allOf(hasName(name), isInstantiated())))).bind("accessSpecDecl"),
                        [&] (const MatchFinder::MatchResult& result) mutable
                        {
                            if (const clang::AccessSpecDecl* decl = result.Nodes.getNodeAs<clang::AccessSpecDecl>("accessSpecDecl"))
                            {
                                if (Matcher::testDecl(*decl, accessSpecDecl(isPublic()), context))
                                {
                                    publicAccess.emplace_back(*decl, AccessSpecifier::Type::Public);
                                }
                                else if (Matcher::testDecl(*decl, accessSpecDecl(isProtected()), context))
                                {
                                    protectedAccess.emplace_back(*decl, AccessSpecifier::Type::Protected);
                                }
                                else
                                {
                                    privateAccess.emplace_back(*decl, AccessSpecifier::Type::Private);
                                }
                            }
                        });
                    /*
                    matcher.addMatcher(accessSpecDecl(hasParent(cxxRecordDecl(allOf(hasName(name), unless(isTemplateInstantiation()))))).bind("accessSpecDecl"),
                        [&] (const MatchFinder::MatchResult& result) mutable
                        {
                            if (const clang::AccessSpecDecl* decl = result.Nodes.getNodeAs<clang::AccessSpecDecl>("accessSpecDecl"))
                            {
                                if (Matcher::testDecl(*decl, accessSpecDecl(hasParent(cxxRecordDecl(isInstantiated()))), context)) return;

                                if (Matcher::testDecl(*decl, accessSpecDecl(isPublic()), context))
                                {
                                    publicAccess.emplace_back(*decl, AccessSpecifier::Type::Public);
                                }
                                else if (Matcher::testDecl(*decl, accessSpecDecl(isProtected()), context))
                                {
                                    protectedAccess.emplace_back(*decl, AccessSpecifier::Type::Protected);
                                }
                                else
                                {
                                    privateAccess.emplace_back(*decl, AccessSpecifier::Type::Private);
                                }
                            }
                        });
                    */
                    /*
                    matcher.addMatcher(accessSpecDecl(hasParent(cxxRecordDecl(allOf(hasName(name), unless(isTemplateInstantiation()))))).bind("accessSpecDecl"),
                        [&] (const MatchFinder::MatchResult& result) mutable
                        {
                            if (const clang::AccessSpecDecl* decl = result.Nodes.getNodeAs<clang::AccessSpecDecl>("accessSpecDecl"))
                            {
                                //if (Matcher::testDecl(*decl, accessSpecDecl(isPublic()), context))
                                if (Matcher::testDecl(*decl, accessSpecDecl(isPublic()), context))
                                {
                                    publicAccess.emplace_back(*decl, AccessSpecifier::Type::Public);
                                }
                                else if (Matcher::testDecl(*decl, accessSpecDecl(isProtected()), context))
                                {
                                    protectedAccess.emplace_back(*decl, AccessSpecifier::Type::Protected);
                                }
                                else
                                {
                                    privateAccess.emplace_back(*decl, AccessSpecifier::Type::Private);
                                }
                            }
                        });
                    */
                    // match all (public, private) constructors
                    /*
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
                    */
                   
                    //matcher.addMatcher(cxxConstructorDecl(hasParent(cxxRecordDecl(allOf(hasName(name), unless(isTemplateInstantiation()))))).bind("constructorDecl"),
                    matcher.addMatcher(cxxConstructorDecl(hasParent(cxxRecordDecl(allOf(hasName(name), isInstantiated())))).bind("constructorDecl"),
                        [&] (const MatchFinder::MatchResult& result) mutable
                        {
                            if (const clang::CXXConstructorDecl* decl = result.Nodes.getNodeAs<clang::CXXConstructorDecl>("constructorDecl"))
                            {
                                if (Matcher::testDecl(*decl, cxxConstructorDecl(isPublic()), context))
                                {
                                    publicConstructors.emplace_back(*decl);
                                }
                                else if (Matcher::testDecl(*decl, cxxConstructorDecl(isProtected()), context))
                                {
                                    protectedConstructors.emplace_back(*decl);
                                }
                                else
                                {
                                    privateConstructors.emplace_back(*decl);
                                }
                            }
                        });
                        
                #undef MATCH

                    matcher.run(context);
                    matcher.clear();
                }
            }

        protected:

            clang::ASTContext& context;

        public:

            using Base::name;
            const clang::CXXRecordDecl& decl;
            clang::SourceRange sourceRange;
            const std::uint32_t numFields;
            std::uint32_t numPublicFields;
            const bool isStruct;
            bool hasNonFundamentalPublicFields;
            bool hasMultiplePublicFieldTypes;
            bool isProxyClassCandidate;
            std::vector<AccessSpecifier> publicAccess;
            std::vector<AccessSpecifier> protectedAccess;
            std::vector<AccessSpecifier> privateAccess;
            std::vector<Constructor> publicConstructors;
            std::vector<Constructor> protectedConstructors;
            std::vector<Constructor> privateConstructors;
            std::vector<MemberField> fields;
            
            CXXClassMetaData(const std::string name, const clang::CXXRecordDecl& decl)
                :
                Base(name, false),
                context(decl.getASTContext()),
                decl(decl),
                sourceRange(decl.getDescribedClassTemplate() ? decl.getDescribedClassTemplate()->getSourceRange() : decl.getSourceRange()),
                numFields(std::distance(decl.field_begin(), decl.field_end())),
                numPublicFields(0),
                isStruct(Matcher::testDecl(decl, clang::ast_matchers::recordDecl(clang::ast_matchers::isStruct()), context)),
                hasNonFundamentalPublicFields(false),
                hasMultiplePublicFieldTypes(false),
                isProxyClassCandidate(numFields > 0 && !(decl.isAbstract() || decl.isPolymorphic() || decl.isEmpty()))
            {
                if (isProxyClassCandidate)
                {
                    collectMetaData();
                }
            }

            virtual void printInfo(clang::SourceManager& sm, const std::string indent = "") const
            {
                std::cout << indent << "C++ " << (isStruct ? "struct: " : "class: ") << name << std::endl;
                std::cout << indent << "\tdefinition: " << sourceRange.printToString(sm) << std::endl;
                std::cout << indent << "\tfields: " << numFields << " (public fields: " << numPublicFields << ")" << std::endl;
                for (auto field : fields)
                {
                    field.printInfo(sm, indent + "\t\t");
                }
                std::cout << indent << "\taccess specifier: " << std::endl;
                for (auto specifier : publicAccess)
                {
                    specifier.printInfo(sm, indent + "\t\t");
                }
                for (auto specifier : protectedAccess)
                {
                    specifier.printInfo(sm, indent + "\t\t");
                }
                for (auto specifier : privateAccess)
                {
                    specifier.printInfo(sm, indent + "\t\t");
                }
            }
        };

        class TemplateClassMetaData : public ClassMetaData
        {
            using Base = ClassMetaData;

            class Declaration
            {
            public:

                const clang::ClassTemplateDecl& decl;
                clang::SourceRange sourceRange;
                const bool isDefinition;

                Declaration(const clang::ClassTemplateDecl& decl)
                    :
                    decl(decl),
                    sourceRange(decl.getSourceRange()),
                    isDefinition(decl.isThisDeclarationADefinition())
                { ; }
            };

            class Definition : public CXXClassMetaData
            {
                using Base = CXXClassMetaData;
                using Base::context;

            public:

                using Base::name;
                using Base::decl;
                using Base::sourceRange;

                Definition(const std::string name, const clang::CXXRecordDecl& decl)
                    :
                    Base(name, decl)
                { ; }

                void printInfo(clang::SourceManager& sm, const std::string indent = "") const
                {
                    Base::printInfo(sm, indent);
                }
            };

        public:
            
            using Base::name;
            Declaration declaration;
            std::vector<Definition> definitions;

            TemplateClassMetaData(const std::string name, const clang::ClassTemplateDecl& decl)
                :
                Base(name, true),
                declaration(decl)
            { 
                if (declaration.isDefinition)
                {
                    definitions.emplace_back(name, *(decl.getTemplatedDecl()));
                }
            }

            bool addDefinition(const clang::CXXRecordDecl& decl, const bool isTemplateSpecialization = false)
            {
                if (!decl.isThisDeclarationADefinition()) return false;

                // isTemplateSpecialization == true: this definition has been instantiated
                // isTemplateSpecialization == false: this declaration is a template class definition and has not been instantiated necessarily

                clang::ClassTemplateDecl* tDecl = decl.getDescribedClassTemplate();
                clang::SourceRange sourceRange = (tDecl != nullptr ? tDecl->getSourceRange() : decl.getSourceRange());

                const std::string className = decl.getNameAsString();
                if (name == className)
                {
                    for (std::size_t i = 0; i < definitions.size(); ++i)
                    {
                        if (sourceRange == definitions[i].sourceRange) 
                        {
                            // if this declaration is a template class definition (not intantiated),
                            // then set template field types only
                            if (!isTemplateSpecialization)
                            {
                                std::size_t fieldId = 0;
                                for (auto field : decl.fields())
                                {
                                    definitions[i].fields[fieldId].typeName = field->getType().getAsString();
                                    ++fieldId;
                                }
                            }

                            return true;
                        }
                    }
                    
                    // no? then add this definition if it is instantiated!
                    if (isTemplateSpecialization)
                    {
                        definitions.emplace_back(className, decl);
                    }

                    return true;
                }

                return false;
            }

            virtual void printInfo(clang::SourceManager& sm, const std::string indent = "") const
            {
                std::cout << "C++ Template Class: " << name << std::endl;
                std::cout << "\tdeclaration is definition: " << (declaration.isDefinition ? "yes" : "no") << std::endl;
                std::cout << "\tdeclaration: " << declaration.sourceRange.printToString(sm) << std::endl;
                std::cout << "\tdefinitions..." << std::endl;
                for (auto definition : definitions)
                {
                    definition.printInfo(sm, "\t\t");
                }
                std::cout << "\t...definitions" << std::endl;
            }
        };
#endif
#endif
    }
}

#endif