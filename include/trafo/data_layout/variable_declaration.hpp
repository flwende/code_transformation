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

        public:

            const clang::VarDecl& decl;
            const clang::SourceRange sourceRange;
            const clang::QualType elementDataType;
            const std::string elementDataTypeName;
            std::string elementDataTypeNamespace;

        protected:

            Declaration(const clang::VarDecl& decl, const clang::QualType elementDataType)
                :
                decl(decl),
                sourceRange(getSourceRangeWithClosingCharacter(decl.getSourceRange(), std::string(";"), decl.getASTContext(), true)),
                elementDataType(elementDataType),
                elementDataTypeName(getDataTypeName(elementDataType))
            {
                if (!elementDataType.isNull())
                {
                    const std::string fullElementTypeName = elementDataType->getCanonicalTypeInternal().getAsString();
                    const std::size_t startPosElementTypeName = fullElementTypeName.find(elementDataTypeName);
                    const std::size_t startPosElementTypeNamespace = fullElementTypeName.rfind(' ', startPosElementTypeName) + 1;

                    elementDataTypeNamespace = fullElementTypeName.substr(startPosElementTypeNamespace, (startPosElementTypeName - startPosElementTypeNamespace));
                }
                else
                {
                    elementDataTypeNamespace = std::string("");
                }
            }

        public:

            virtual ~Declaration() { ; }

            virtual bool isScalarTypeDeclaration() const { return true; }

            virtual std::uint32_t getNestingLevel() const { return 0; }

            virtual const std::vector<std::size_t>& getExtent() const = 0;

            virtual const std::vector<std::string>& getExtentString() const = 0;

            virtual void printInfo(const clang::SourceManager& sourceManager, const std::string indent = std::string("")) const
            {
                std::cout << indent << "* variable name: " << decl.getNameAsString() << std::endl;
                std::cout << indent << "* range: " << sourceRange.printToString(sourceManager) << std::endl;
                std::cout << indent << "* element data type: " << elementDataType.getAsString();
                if (elementDataType.getAsString() != elementDataTypeName)
                {
                    std::cout << " (" <<  elementDataTypeNamespace << elementDataTypeName << ")";
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
            using Base::elementDataTypeNamespace;

            const clang::QualType arrayType;
            const bool isNested;
            const std::uint32_t nestingLevel;
            const std::vector<std::size_t> extent;
            const std::vector<std::string> extentString;

            ConstantArrayDeclaration(const clang::VarDecl& decl, const bool isNested, const std::uint32_t nestingLevel, const clang::QualType elementDataType, const std::vector<std::size_t>& extent, const std::vector<std::string>& extentString)
                :
                Base(decl, elementDataType),
                arrayType(decl.getType()),
                isNested(isNested),
                nestingLevel(nestingLevel),
                extent(extent),
                extentString(extentString)
            { ; }

            ~ConstantArrayDeclaration() { ; }

            bool isScalarTypeDeclaration() const { return true; }

            std::uint32_t getNestingLevel() const { return nestingLevel; }

            const std::vector<std::size_t>& getExtent() const { return extent; }

            const std::vector<std::string>& getExtentString() const { return extentString; }

            static ConstantArrayDeclaration make(const clang::VarDecl& decl, clang::ASTContext& context)
            {
                clang::QualType elementDataType;
                bool isNested = false;
                std::uint32_t nestingLevel = 0;
                std::vector<std::size_t> extent;
                std::vector<std::string> extentString;

                if (const clang::Type* type = decl.getType().getTypePtrOrNull())
                {
                    if (type->isConstantArrayType())
                    {
                        do
                        {
                            // in all cases 
                            const clang::ConstantArrayType* arrayType = reinterpret_cast<const clang::ConstantArrayType*>(type);
                            clang::QualType qualType = arrayType->getElementType();

                            //extent.push_back(arrayType->getSize().getLimitedValue());
                            std::size_t value = arrayType->getSize().getLimitedValue();
                            extent.insert(extent.begin(), value);
                            extentString.insert(extentString.begin(), std::to_string(value));
                            type = qualType.getTypePtrOrNull();

                            if (type->isConstantArrayType())
                            {
                                isNested |= true;
                                ++nestingLevel;
                                continue;
                            }

                            elementDataType = qualType;
                            break;
                        } 
                        while (type);
                    }
                    else
                    {
                        std::cerr << "error: this is not a constant array declaration" << std::endl;
                    }
                    
                }

                return ConstantArrayDeclaration(decl, isNested, nestingLevel, elementDataType, extent, extentString);
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
                std::cout << " -> ";
                for (std::size_t i = 0; i < extentString.size(); ++i)
                {
                    std::cout << "[" << extentString[i] << "]";
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
            using Base::elementDataTypeNamespace;

            const clang::QualType containerType;
            const bool isNested;
            const std::uint32_t nestingLevel;
            const std::vector<std::size_t> extent;
            const std::vector<std::string> extentString;

            ContainerDeclaration(const clang::VarDecl& decl, const bool isNested, const std::uint32_t nestingLevel, const clang::QualType elementDataType, const std::vector<std::size_t>& extent, const std::vector<std::string>& extentString)
                :
                Base(decl, elementDataType),
                containerType(decl.getType()),
                isNested(isNested),
                nestingLevel(nestingLevel),
                extent(extent),
                extentString(extentString)
            { ; }

            ~ContainerDeclaration() { ; }

            bool isScalarTypeDeclaration() const { return true; }

            std::uint32_t getNestingLevel() const { return nestingLevel; }

            const std::vector<std::size_t>& getExtent() const { return extent; }

            const std::vector<std::string>& getExtentString() const { return extentString; }

            static ContainerDeclaration make(const clang::VarDecl& decl, clang::ASTContext& context, const std::vector<std::string>& containerNames)
            {
                clang::QualType elementDataType;
                const clang::SourceRange sourceRange = getSourceRangeWithClosingCharacter(decl.getSourceRange(), std::string(";"), decl.getASTContext(), true);
                std::string fullName = decl.getType().getAsString();
                std::string name = decl.getType()->getAsRecordDecl()->getNameAsString();
                const clang::Type* type = decl.getType().getTypePtrOrNull();
                bool isNested = false;
                std::uint32_t nestingLevel = 0;
                std::vector<std::size_t> extent;
                std::vector<std::string> extentString;

                // check for nested container declaration
                // note: in the first instance 'type' is either a class or structur type (it is the container type itself)
                while (type)
                {
                    // we can do this as any variable declaration of the container is a specialzation of containerType<T,..>
                    if (const clang::TemplateSpecializationType* const tsType = type->getAs<clang::TemplateSpecializationType>())
                    {
                        // if the container is an array of fixed size...
                        std::size_t value = 0;
                        std::string valueString("");
                        if (name == std::string("array"))
                        {
                            // get the extent: 2nd template parameter
                            if (tsType->getArg(1).getKind() == clang::TemplateArgument::ArgKind::Integral)
                            {
                                value = tsType->getArg(1).getAsIntegral().getExtValue();
                            }
                            else if (tsType->getArg(1).getKind() == clang::TemplateArgument::ArgKind::Expression)
                            {
                                const clang::Expr* const expr = tsType->getArg(1).getAsExpr();
                                value = (expr ? expr->EvaluateKnownConstInt(context).getExtValue() : 0);
                            }

                            valueString = std::to_string(value);
                        }
                        else if (name == std::string("vector"))
                        {
                            std::string declString = dumpSourceRangeToString(sourceRange, decl.getASTContext().getSourceManager());                        
                            std::size_t firstArgumentBeginPos = declString.find('(', declString.find(decl.getNameAsString()));                            
                            std::size_t firstArgumentEndPos = declString.find(',', firstArgumentBeginPos);
                            if (firstArgumentEndPos == std::string::npos)
                            {
                                firstArgumentEndPos = declString.find(')', firstArgumentBeginPos);
                            }

                            if (firstArgumentBeginPos != std::string::npos && firstArgumentEndPos != std::string::npos)
                            {
                                value = 1; // TODO: remove that! this is just to indicate that there is an argument
                                valueString = declString.substr(firstArgumentBeginPos + 1, firstArgumentEndPos - firstArgumentBeginPos - 1);
                            }
                        }
                        extent.insert(extent.begin(), value);
                        extentString.insert(extentString.begin(), valueString);
                        
                        // the first template argument is the type of the content of the container
                        clang::QualType taQualType = tsType->getArg(0).getAsType();

                        if (const clang::Type* const taType = taQualType.getTypePtrOrNull())
                        {
                            // if it is a class or structure type, check for it being a containerType
                            // TEST: if (taType->isRecordType())
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

                return ContainerDeclaration(decl, isNested, nestingLevel, elementDataType, extent, extentString);
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
                for (std::size_t i = 0; i < extentString.size(); ++i)
                {
                    std::cout << "[" << extentString[i] << "]";
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