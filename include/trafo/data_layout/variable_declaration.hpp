// Copyright (c) 2017-2018 Florian Wende (flwende@gmail.com)
//
// Distributed under the BSD 2-clause Software License
// (See accompanying file LICENSE)

#if !defined(TRAFO_DATA_LAYOUT_VARIABLE_DECLARATION_HPP)
#define TRAFO_DATA_LAYOUT_VARIABLE_DECLARATION_HPP

#include <cstdint>
#include <iostream>
#include <vector>
#include <misc/ast_helper.hpp>

#if !defined(TRAFO_NAMESPACE)
    #define TRAFO_NAMESPACE fw
#endif

namespace TRAFO_NAMESPACE
{
    namespace internal
    {
        class Declaration
        {
            std::string getDataTypeName(const clang::QualType& dataType)
            {
                if (const clang::Type* const type = dataType.getTypePtrOrNull())
                {
                    if (type->isClassType() || type->isStructureType())
                    {
                        const clang::CXXRecordDecl& decl = *(type->getAsCXXRecordDecl());
                        return decl.getNameAsString();
                    }
                }

                return dataType.getAsString();
            }

        protected:

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

        public:

            virtual ~Declaration() { ; }

            virtual void printInfo(const clang::SourceManager& sourceManager, const std::string indent = std::string("")) const
            {
                std::cout << indent << "* variable name: " << decl.getNameAsString() << std::endl;
                std::cout << indent << "* range: " << sourceRange.printToString(sourceManager) << std::endl;
                std::cout << indent << "* element data type: " << elementDataType.getAsString();
                if (elementDataType.getAsString() != elementDataTypeName)
                {
                    std::cout << " (" << elementDataTypeName << ")";
                }
                std::cout << std::endl;
            }
        };

        class ConstantArrayDeclaration : public Declaration
        {
            using Base = Declaration;

        public:

            using Base::decl;
            using Base::sourceRange;
            using Base::elementDataType;
            using Base::elementDataTypeName;

            const clang::QualType arrayType;
            const bool isNested;
            const std::uint32_t nestingLevel;
            const std::vector<std::size_t> extent;

            ConstantArrayDeclaration(const clang::VarDecl& decl, const bool isNested, const std::uint32_t nestingLevel, const clang::QualType elementDataType, const std::vector<std::size_t>& extent)
                :
                Base(decl, elementDataType),
                arrayType(decl.getType()),
                isNested(isNested),
                nestingLevel(nestingLevel),
                extent(extent)
            { ; }

            ~ConstantArrayDeclaration() { ; }

            static ConstantArrayDeclaration make(const clang::VarDecl& decl, const clang::ConstantArrayType* arrayType, clang::ASTContext& context)
            {
                clang::QualType qualType = arrayType->getElementType();  
                const clang::Type* type = qualType.getTypePtrOrNull();
                clang::QualType elementDataType;
                bool isNested = false;
                std::uint32_t nestingLevel = 0;
                std::vector<std::size_t> extent;

                while (type)
                {
                    extent.push_back(arrayType->getSize().getLimitedValue());

                    if (type->isConstantArrayType())
                    {
                        isNested |= true;
                        ++nestingLevel;
                        arrayType = reinterpret_cast<const clang::ConstantArrayType*>(type); // maybe unsafe: getAs<> does not work here!
                        qualType = arrayType->getElementType();
                        type = qualType.getTypePtrOrNull();
                        continue;
                    }

                    elementDataType = qualType;

                    // break condition for the outer loop!
                    // this point is reached only if the current type is not a constant array type
                    type = nullptr;

                    break;
                }

                return ConstantArrayDeclaration(decl, isNested, nestingLevel, elementDataType, extent);
            }

            virtual void printInfo(const clang::SourceManager& sourceManager, const std::string indent = std::string("")) const
            {
                std::cout << indent << "CONSTANT ARRAY DECLARATION" << std::endl;

                Base::printInfo(sourceManager, indent + std::string("\t"));
                
                std::cout << indent << "\t\t+-> declaration: " << decl.getSourceRange().printToString(sourceManager) << std::endl;
                std::cout << indent << "\t\t+-> type: " << arrayType.getAsString() << std::endl;
                std::cout << indent << "\t\t+-> extent: ";
                for (std::size_t i = 0; i < extent.size(); ++i)
                {
                    std::cout << "[" << (extent[i] > 0 ? std::to_string(extent[i]) : std::string("-")) << "]";
                }
                std::cout << std::endl;
                std::cout << indent << "\t\t+-> nested: " << (isNested ? "yes" : "no") << std::endl;
                if (isNested)
                {
                    std::cout << indent << "\t\t\t+-> nesting level: " << nestingLevel << std::endl;
                }
            }
        };
        

        class ContainerDeclaration : public Declaration
        {
            using Base = Declaration;

        public:

            using Base::decl;
            using Base::sourceRange;
            using Base::elementDataType;
            using Base::elementDataTypeName;

            const clang::QualType containerType;
            const bool isNested;
            const std::uint32_t nestingLevel;
            const std::vector<std::size_t> extent;

            ContainerDeclaration(const clang::VarDecl& decl, const bool isNested, const std::uint32_t nestingLevel, const clang::QualType elementDataType, const std::vector<std::size_t>& extent)
                :
                Base(decl, elementDataType),
                containerType(decl.getType()),
                isNested(isNested),
                nestingLevel(nestingLevel),
                extent(extent)
            { ; }

            ~ContainerDeclaration() { ; }

            static ContainerDeclaration make(const clang::VarDecl& decl, clang::ASTContext& context, const std::vector<std::string>& containerNames)
            {
                clang::QualType elementDataType;
                std::string name = decl.getType().getAsString();
                const clang::Type* type = decl.getType().getTypePtrOrNull();
                bool isNested = false;
                std::uint32_t nestingLevel = 0;
                std::vector<std::size_t> extent;

                // check for nested container declaration
                // note: in the first instance 'type' is either a class or structur type (it is the container type itself)
                while (type)
                {
                    // we can do this as any variable declaration of the container is a specialzation of containerType<T,..>
                    if (const clang::TemplateSpecializationType* const tsType = type->getAs<clang::TemplateSpecializationType>())
                    {
                        // if the container is an array of fixed size...
                        if (name == std::string("array"))
                        {
                            const clang::Expr* const expr = tsType->getArg(1).getAsExpr();
                            // get the extent: 2nd template parameter
                            extent.push_back(expr ? expr->EvaluateKnownConstInt(context).getExtValue() : 0);
                        }
                        else
                        {
                            extent.push_back(0);
                        }

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
                                    bool isContainerType = false;

                                    for (const auto& containerName : containerNames)
                                    {
                                        name = cxxRecordDecl->getNameAsString();

                                        if (name == containerName)
                                        {
                                            isNested |= true;
                                            ++nestingLevel;
                                            type = taType;
                                            isContainerType = true;                                            
                                            break;
                                        }
                                    }

                                    if (isContainerType) continue;
                                }
                            }
                        }

                        // this point is reached if the a non-vector type has been encountered
                        elementDataType = taQualType;
                    }

                    // break condition for the outer loop!
                    // this point is reached only if the current template argument type is not the specified container type
                    type = nullptr;
                }

                return ContainerDeclaration(decl, isNested, nestingLevel, elementDataType, extent);
            }

            virtual void printInfo(const clang::SourceManager& sourceManager, const std::string indent = std::string("")) const
            {
                std::cout << indent << "CONTAINER DECLARATION" << std::endl;

                Base::printInfo(sourceManager, indent + std::string("\t"));
                
                std::cout << indent << "\t* container type: " << containerType.getAsString() << std::endl;
                std::cout << indent << "\t\t+-> declaration: " << decl.getSourceRange().printToString(sourceManager) << std::endl;
                std::cout << indent << "\t\t+-> extent: ";
                for (std::size_t i = 0; i < extent.size(); ++i)
                {
                    std::cout << "[" << (extent[i] > 0 ? std::to_string(extent[i]) : std::string("-")) << "]";
                }
                std::cout << std::endl;
                std::cout << indent << "\t\t+-> nested: " << (isNested ? "yes" : "no") << std::endl;
                if (isNested)
                {
                    std::cout << indent << "\t\t\t+-> nesting level: " << nestingLevel << std::endl;
                }
            }
        };
    }
}

#endif