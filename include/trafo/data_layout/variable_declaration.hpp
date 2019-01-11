// Copyright (c) 2017-2018 Florian Wende (flwende@gmail.com)
//
// Distributed under the BSD 2-clause Software License
// (See accompanying file LICENSE)

#if !defined(TRAFO_DATA_LAYOUT_VARIABLE_DECLARATION_HPP)
#define TRAFO_DATA_LAYOUT_VARIABLE_DECLARATION_HPP

#include <iostream>
#include <string>
#include <cstdint>
#include <misc/matcher.hpp>
#include <misc/ast_helper.hpp>

#if !defined(TRAFO_NAMESPACE)
    #define TRAFO_NAMESPACE fw
#endif

namespace TRAFO_NAMESPACE
{
    namespace internal
    {
        struct Declaration
        {
            const clang::VarDecl& decl;
            const clang::SourceRange sourceRange;
            const clang::QualType elementDataType;
            const std::string elementDataTypeName;

            Declaration(const clang::VarDecl& decl, const clang::QualType elementDataType)
                :
                decl(decl),
                sourceRange(decl.getSourceRange()),
                elementDataType(elementDataType),
                elementDataTypeName(getDataTypeName(elementDataType))
            { ; }

            void print(const clang::SourceManager& sourceManager) const
            {
                std::cout << "\t* variable name: " << decl.getNameAsString() << std::endl;
                std::cout << "\t* range: " << sourceRange.printToString(sourceManager) << std::endl;
                std::cout << "\t* element data type: " << elementDataType.getAsString();
                if (elementDataType.getAsString() != elementDataTypeName)
                {
                    std::cout << " (" << elementDataTypeName << ")";
                }
                std::cout << std::endl;
            }
        };

        struct ContainerDeclaration : public Declaration
        {
            using Base = Declaration;
            using Base::decl;
            using Base::sourceRange;
            using Base::elementDataType;
            using Base::elementDataTypeName;

            const clang::QualType containerType;
            const bool isNested;
            const std::uint32_t nestingLevel;

            ContainerDeclaration(const clang::VarDecl& decl, const bool isNested, const std::uint32_t nestingLevel, const clang::QualType elementDataType)
                :
                Base(decl, elementDataType),
                containerType(decl.getType()),
                isNested(isNested),
                nestingLevel(nestingLevel)
            { ; }

            static ContainerDeclaration make(const clang::VarDecl& decl, clang::ASTContext& context, const std::string containerType)
            {
                using namespace clang::ast_matchers;

                clang::QualType innerMostType;
                const clang::Type* type = decl.getType().getTypePtrOrNull();
                bool isNested = false;
                std::uint32_t nestingLevel = 0;

                // check for nested container declaration
                // note: in the first instance 'type' is either a class or structur type (it is the container type itself)
                while (type)
                {
                    // we can do this as any variable declaration of the container is a specialzation of containerType<T,..>
                    if (const clang::TemplateSpecializationType* const tsType = type->getAs<clang::TemplateSpecializationType>())
                    {
                        // the first template argument is the type of the content of the container
                        clang::QualType taQualType = tsType->getArg(0).getAsType();
                        if (const clang::Type* const taType = taQualType.getTypePtrOrNull())
                        {
                            // if it is a class or structure type, check for it being a containerType
                            if (taType->isClassType() || taType->isStructureType())
                            {
                                // if it is a container, get nesting information and continue the loop execution
                                if (const clang::CXXRecordDecl* const cxxRecordDecl = taType->getAsCXXRecordDecl())
                                {
                                    // if it is a container, get nesting information and continue the loop execution
                                    if (cxxRecordDecl->getNameAsString() == containerType)
                                    {
                                        isNested |= true;
                                        ++nestingLevel;
                                        type = taType;
                                        continue;
                                    }
                                }
                            }
                        }
                        // this point is reached if the a non-vector type has been encountered
                        innerMostType = taQualType;
                    }

                    // break condition for the outer loop!
                    // this point is reached only if the current template argument type is not the specified container type
                    type = nullptr;
                }

                return ContainerDeclaration(decl, isNested, nestingLevel, innerMostType);
            }

            void printInfo(const clang::SourceManager& sourceManager) const
            {
                Base::print(sourceManager);
                
                std::cout << "\t* container type: " << containerType.getAsString() << std::endl;
                std::cout << "\t\t+-> declaration: " << decl.getSourceRange().printToString(sourceManager) << std::endl;
                std::cout << "\t\t+-> nested: " << (isNested ? "yes" : "no") << std::endl;
                if (isNested)
                {
                    std::cout << "\t\t\t+-> nesting level: " << nestingLevel << std::endl;
                }
            }
        };
    }
}

#endif