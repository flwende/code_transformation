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
    }
}

#endif