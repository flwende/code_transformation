// Copyright (c) 2017-2018 Florian Wende (flwende@gmail.com)
//
// Distributed under the BSD 2-clause Software License
// (See accompanying file LICENSE)

#if !defined(MISC_AST_HELPER_HPP)
#define MISC_AST_HELPER_HPP

#include <string>
#include <cstdint>
#include <clang/AST/AST.h>
#include <misc/string_helper.hpp>

#if !defined(TRAFO_NAMESPACE)
    #define TRAFO_NAMESPACE fw
#endif

namespace TRAFO_NAMESPACE
{
    namespace internal
    {
        static std::string getDataTypeName(const clang::QualType& dataType)
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

        struct ClassDecl
        {
            static const clang::CXXRecordDecl* const getTemplatedDecl(const clang::CXXRecordDecl& decl)
            {
                return &decl;
            }

            static const clang::CXXRecordDecl* const getTemplatedDecl(const clang::ClassTemplateDecl& decl)
            {
                return decl.getTemplatedDecl();
            }

            static const clang::ClassTemplateDecl* const getDescribedClassTemplate(const clang::ClassTemplateDecl& decl)
            {
                return &decl;
            }

            static const clang::ClassTemplateDecl* const getDescribedClassTemplate(const clang::CXXRecordDecl& decl)
            {
                return decl.getDescribedClassTemplate();
            }
        };

        struct CompareSourceRange
        {
            bool operator() (const clang::SourceRange& lhs, const clang::SourceRange& rhs) const
            {
                return lhs.getBegin() < rhs.getBegin(); 
            }
        };

        static std::uint32_t getSpellingLineNumber(const clang::SourceLocation& sourceLocation, clang::ASTContext& context)
        {
            return context.getFullLoc(sourceLocation).getSpellingLineNumber();
        }

        static std::uint32_t getSpellingColumnNumber(const clang::SourceLocation& sourceLocation, clang::ASTContext& context)
        {
            return context.getFullLoc(sourceLocation).getSpellingColumnNumber();
        }

        static clang::SourceLocation getBeginOfLine(const clang::SourceLocation& sourceLocation, clang::ASTContext& context)
        {
            const clang::SourceManager& sourceManager = context.getSourceManager();
            const clang::FileID fileId = sourceManager.getFileID(sourceLocation);
            const std::uint32_t sourceLocationLineNumber = getSpellingLineNumber(sourceLocation, context);

            return sourceManager.translateLineCol(fileId, sourceLocationLineNumber, 1);
        }

        static clang::SourceLocation getNextLine(const clang::SourceLocation& sourceLocation, clang::ASTContext& context)
        {
            const clang::SourceManager& sourceManager = context.getSourceManager();
            const clang::FileID fileId = sourceManager.getFileID(sourceLocation);
            const std::uint32_t sourceLocationLineNumber = getSpellingLineNumber(sourceLocation, context);
            const clang::SourceLocation nextLine = sourceManager.translateLineCol(fileId, sourceLocationLineNumber + 1, 1);

            if (sourceLocationLineNumber == getSpellingLineNumber(nextLine, context))
            {
                return sourceLocation.getLocWithOffset(1);
            }
            else
            {
                return nextLine;
            }
        }

        static clang::SourceLocation getLocationOfFirstOccurence(const clang::SourceRange& sourceRange, clang::ASTContext& context, const char character, const std::uint32_t lineOffset = 0, const std::uint32_t colOffset = 0)
        {
            const clang::SourceManager& sourceManager = context.getSourceManager();
            
            // get full source location of 'decl' start
            const clang::SourceLocation declSourceLocation = sourceRange.getBegin();
            const clang::FullSourceLoc fullDeclSourceLocation = context.getFullLoc(declSourceLocation);
            const std::uint32_t lineDecl = fullDeclSourceLocation.getSpellingLineNumber();

            // get full source location of FILE end
            const clang::FileID fid = sourceManager.getFileID(declSourceLocation);
            const clang::SourceLocation EOFLocation = sourceManager.getLocForEndOfFile(fid);
            const clang::FullSourceLoc fullEOFLocation = context.getFullLoc(EOFLocation);
            const std::uint32_t lineEOF = fullEOFLocation.getSpellingLineNumber();

            // default return value: invalid result location!
            clang::SourceLocation result;
            
            if (const char* linePtr = sourceManager.getCharacterData(sourceManager.translateLineCol(fid, lineDecl, 1)))
            {
                // this string contains the entire source code until the end of this FILE
                const std::string sourceText(linePtr);
                std::istringstream sourceTextStream(sourceText);
                std::string thisLine;
                
                // process the source code line by line
                const std::uint32_t iStart = lineDecl + lineOffset;
                for (std::uint32_t i = lineDecl; i < lineEOF; ++i)
                {
                    if (std::getline(sourceTextStream, thisLine, '\n'))
                    {
                        if (i < iStart) continue;

                        const std::string::size_type pos = thisLine.find(character, (i == iStart ? colOffset : 0));
                        // stop, if the opening brace has been found
                        if (pos != std::string::npos)
                        {  
                            return sourceManager.translateLineCol(fid, i, pos + 1);
                        }
                    }
                }
            }

            return result;
        }

        static clang::SourceRange getSourceRangeWithClosingCharacter(const clang::SourceRange& sourceRange, const char character, clang::ASTContext& context, const bool withIndentation = true)
        {
            if (!sourceRange.isValid()) return sourceRange;

            const clang::SourceLocation beginLocation = sourceRange.getBegin();
            const clang::SourceLocation endLocation = sourceRange.getEnd();
            const std::uint32_t lineBegin = getSpellingLineNumber(beginLocation, context);
            const std::uint32_t lineEnd = getSpellingLineNumber(endLocation, context);
            const std::uint32_t columnEnd = getSpellingColumnNumber(endLocation, context);
            const clang::SourceLocation colonLocation = getLocationOfFirstOccurence(sourceRange, context, character, (lineEnd - lineBegin), columnEnd);

            return clang::SourceRange(withIndentation ? beginLocation : getBeginOfLine(beginLocation, context), colonLocation);
        }
    }
}

#endif