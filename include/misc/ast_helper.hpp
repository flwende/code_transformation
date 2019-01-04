// Copyright (c) 2017-2018 Florian Wende (flwende@gmail.com)
//
// Distributed under the BSD 2-clause Software License
// (See accompanying file LICENSE)

#if !defined(MISC_AST_HELPER_HPP)
#define MISC_AST_HELPER_HPP

#include <string>
#include <clang/AST/AST.h>

#if !defined(TRAFO_NAMESPACE)
    #define TRAFO_NAMESPACE fw
#endif

namespace TRAFO_NAMESPACE
{
    namespace internal
    {
        static std::string getDataTypeName(const clang::QualType& dataType)
        {
            const clang::Type* type = dataType.getTypePtrOrNull();
            if (type != nullptr && (type->isClassType() || type->isStructureType()))
            {
                clang::CXXRecordDecl* decl = type->getAsCXXRecordDecl();
                return decl->getNameAsString();
            }
            return dataType.getAsString();
        }

        /*
        template <typename T>
        struct ClassDecl
        {
            static const clang::CXXRecordDecl* getTemplatedDecl(const T& decl)
            {
                return &decl;
            }

            static const clang::ClassTemplateDecl* getDescribedClassTemplate(const T& decl)
            {
                return &decl;
            }
        };

        template <>
        struct ClassDecl<clang::ClassTemplateDecl>
        {
            static const clang::CXXRecordDecl* getTemplatedDecl(const clang::ClassTemplateDecl& decl)
            {
                return decl.getTemplatedDecl();
            }

            static const clang::ClassTemplateDecl* getDescribedClassTemplate(const clang::CXXRecordDecl& decl)
            {
                return decl.getDescribedClassTemplate();
            }
        };
        */
        struct ClassDecl
        {
            static const clang::CXXRecordDecl* getTemplatedDecl(const clang::CXXRecordDecl& decl)
            {
                return &decl;
            }

            static const clang::CXXRecordDecl* getTemplatedDecl(const clang::ClassTemplateDecl& decl)
            {
                return decl.getTemplatedDecl();
            }

            static const clang::ClassTemplateDecl* getDescribedClassTemplate(const clang::ClassTemplateDecl& decl)
            {
                return &decl;
            }

            static const clang::ClassTemplateDecl* getDescribedClassTemplate(const clang::CXXRecordDecl& decl)
            {
                return decl.getDescribedClassTemplate();
            }
        };
    }
}

#endif