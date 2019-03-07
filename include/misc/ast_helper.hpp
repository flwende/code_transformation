// Copyright (c) 2017-2018 Florian Wende (flwende@gmail.com)
//
// Distributed under the BSD 2-clause Software License
// (See accompanying file LICENSE)

#if !defined(MISC_AST_HELPER_HPP)
#define MISC_AST_HELPER_HPP

#include <cstdint>
#include <memory>
#include <string>
#include <clang/AST/AST.h>
#include <clang/Lex/Lexer.h>
#include <clang/Lex/Preprocessor.h>
#include <misc/string_helper.hpp>

#if !defined(TRAFO_NAMESPACE)
    #define TRAFO_NAMESPACE fw
#endif

namespace TRAFO_NAMESPACE
{
    namespace internal
    {  
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

        static std::uint32_t getExpansionLineNumber(const clang::SourceLocation& sourceLocation, clang::ASTContext& context)
        {
            return context.getFullLoc(sourceLocation).getExpansionLineNumber();
        }

        static std::uint32_t getExpansionColumnNumber(const clang::SourceLocation& sourceLocation, clang::ASTContext& context)
        {
            return context.getFullLoc(sourceLocation).getExpansionColumnNumber();
        }

        static clang::SourceLocation getSpellingSourceLocation(const clang::SourceLocation& sourceLocation, clang::ASTContext& context)
        {
            return context.getFullLoc(sourceLocation).getSpellingLoc();
        }

        static clang::SourceLocation getExpansionSourceLocation(const clang::SourceLocation& sourceLocation, clang::ASTContext& context)
        {
            return context.getFullLoc(sourceLocation).getExpansionLoc();
        }

        static clang::SourceLocation getBeginOfLine(const clang::SourceLocation& sourceLocation, clang::ASTContext& context)
        {
            const clang::SourceManager& sourceManager = context.getSourceManager();
            const clang::FileID fileId = sourceManager.getFileID(getSpellingSourceLocation(sourceLocation, context));
            const std::uint32_t sourceLocationLineNumber = getSpellingLineNumber(sourceLocation, context);

            return sourceManager.translateLineCol(fileId, sourceLocationLineNumber, 1);
        }

        static clang::SourceLocation getNextLine(const clang::SourceLocation& sourceLocation, clang::ASTContext& context)
        {
            const clang::SourceManager& sourceManager = context.getSourceManager();
            const clang::FileID fileId = sourceManager.getFileID(getSpellingSourceLocation(sourceLocation, context));
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

        static clang::SourceRange getSpellingSourceRange(const clang::SourceRange& sourceRange, clang::ASTContext& context)
        {
            const clang::SourceLocation spellingBeginLoc = getSpellingSourceLocation(sourceRange.getBegin(), context);
            const clang::SourceLocation spellingEndLoc = getSpellingSourceLocation(sourceRange.getEnd(), context);

            return clang::SourceRange(spellingBeginLoc, spellingEndLoc);
        }

        static clang::SourceRange getExpansionSourceRange(const clang::SourceRange& sourceRange, clang::ASTContext& context)
        {
            const clang::SourceLocation expansionBeginLoc = getExpansionSourceLocation(sourceRange.getBegin(), context);
            const clang::SourceLocation expansionEndLoc = getExpansionSourceLocation(sourceRange.getEnd(), context);

            if (expansionBeginLoc == expansionEndLoc)
            {
                return clang::SourceRange(expansionBeginLoc, getNextLine(expansionBeginLoc, context).getLocWithOffset(-1));
            }
            else
            {
                return clang::SourceRange(expansionBeginLoc, expansionEndLoc);
            }
        }

        static clang::SourceRange extendSourceRangeByLines(const clang::SourceRange& sourceRange, const std::size_t numLines, clang::ASTContext& context)
        {
            const clang::SourceLocation beginLoc = sourceRange.getBegin();
            const clang::SourceManager& sourceManager = context.getSourceManager();
            const clang::FileID fileId = sourceManager.getFileID(getSpellingSourceLocation(beginLoc, context));

            const std::uint32_t endLocLineNumber = getSpellingLineNumber(sourceRange.getEnd(), context);
            const clang::SourceLocation endLoc = sourceManager.translateLineCol(fileId, endLocLineNumber + numLines, 1);
            const clang::SourceRange extendedSourceRange(beginLoc, endLoc);

            if (extendedSourceRange.isValid()) return extendedSourceRange;

            return sourceRange;
        }

        /*
        static std::string dumpStmtToString(const clang::Stmt* const stmt, const clang::SourceManager& sourceManager)
        {
            // return empty string if 'stmt' is invalid
            if (!stmt) return std::string("");
            
            std::string buffer; // will hold the content
            llvm::raw_string_ostream streamBuffer(buffer); // streamBuffer using 'buffer' as internal storage

            // note: 'const' reference is casted away (looks like the signature of 'dump' is inappropriate)!
            stmt->dump(streamBuffer, const_cast<clang::SourceManager&>(sourceManager)); // dump content of 'stmt' to 'streamBuffer'
            
            return streamBuffer.str(); // get content of 'streamBuffer'
        }
        
        static std::string dumpDeclToString(const clang::Decl* const decl)
        {
            // return empty string if 'decl' is invalid
            if (!decl) return std::string("");
            
            std::string buffer; // will hold the content
            llvm::raw_string_ostream streamBuffer(buffer); // streamBuffer using 'buffer' as internal storage

            decl->dump(streamBuffer); // dump content of 'decl' to 'streamBuffer'

            return streamBuffer.str(); // get content of 'streamBuffer'
        }
        */
        static std::string dumpStmtToStringHumanReadable(const clang::Stmt* const stmt, const clang::LangOptions& langOpts, const bool insertLeadingNewline)
        {
            // return empty string if 'stmt' is invalid
            if (!stmt) return std::string("");
            
            std::string buffer; // will hold the content
            llvm::raw_string_ostream streamBuffer(buffer); // streamBuffer using 'buffer' as internal storage

            stmt->printPretty(streamBuffer, nullptr, clang::PrintingPolicy(langOpts), 0); // dump content of 'stmt' to 'streamBuffer'

            return (insertLeadingNewline ? std::string("\n") : std::string("")) + streamBuffer.str(); // get content of 'streamBuffer'
        }

        static std::string dumpStmtToStringHumanReadable(const clang::Stmt* const stmt, const bool insertLeadingNewline)
        {
            return dumpStmtToStringHumanReadable(stmt, clang::LangOptions(), insertLeadingNewline);
        }
        /*
        static std::string dumpDeclToStringHumanReadable(const clang::Decl* const decl, const clang::LangOptions& langOpts, const bool insertLeadingNewline)
        {
            // return empty string if 'decl' is invalid
            if (!decl) return std::string("");
            
            std::string buffer; // will hold the content
            llvm::raw_string_ostream streamBuffer(buffer); // streamBuffer using 'buffer' as internal storage

            decl->print(streamBuffer, clang::PrintingPolicy(langOpts), 0); // dump content of 'decl' to 'streamBuffer'

            return (insertLeadingNewline ? std::string("\n") : std::string("")) + streamBuffer.str(); // get content of 'streamBuffer'
        }

        static std::string dumpDeclToStringHumanReadable(const clang::Decl* const decl, const bool insertLeadingNewline)
        {
            return dumpDeclToStringHumanReadable(decl, clang::LangOptions(), insertLeadingNewline);
        }
        */

        static std::string dumpSourceRangeToString(const clang::SourceRange sourceRange, const clang::SourceManager& sourceManager, const clang::LangOptions& langOpts)
        {
            if (!sourceRange.isValid()) return std::string("");

            const llvm::StringRef sourceText = clang::Lexer::getSourceText(clang::CharSourceRange::getCharRange(sourceRange), sourceManager, langOpts);

            return sourceText.str();
        }

        static std::string dumpSourceRangeToString(const clang::SourceRange sourceRange, const clang::SourceManager& sourceManager)
        {
            return dumpSourceRangeToString(sourceRange, sourceManager, clang::LangOptions());
        }
        
        static std::string dumpContainingLineToString(const clang::SourceLocation& sourceLocation, clang::ASTContext& context)
        {
            const clang::SourceLocation beginOfLine = getBeginOfLine(sourceLocation, context);
            const clang::SourceLocation endOfLine = getNextLine(sourceLocation, context).getLocWithOffset(-1);

            return dumpSourceRangeToString(clang::SourceRange(beginOfLine, endOfLine), context.getSourceManager());
        }

        static clang::SourceLocation getLocationOfFirstOccurence(const clang::SourceRange& sourceRange, clang::ASTContext& context, const char character, const std::uint32_t lineOffset = 0, const std::uint32_t colOffset = 0)
        {
            const clang::SourceManager& sourceManager = context.getSourceManager();
            
            // get full source location of 'decl' start
            const clang::SourceLocation declSourceLocation = sourceRange.getBegin();
            const clang::FullSourceLoc fullDeclSourceLocation = context.getFullLoc(declSourceLocation);
            const std::uint32_t lineDecl = fullDeclSourceLocation.getSpellingLineNumber();

            // get full source location of FILE end
            const clang::FileID fid = sourceManager.getFileID(getSpellingSourceLocation(declSourceLocation, context));
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

        static bool isThisAMacroExpansion(const clang::NamedDecl& decl)
        {
            clang::ASTContext& context = decl.getASTContext();
            const std::uint32_t declSpellingLineNumberBegin = getSpellingLineNumber(decl.getBeginLoc(), context);
            const std::uint32_t declExpansionLineNumberBegin = getExpansionLineNumber(decl.getBeginLoc(), context);

            if (declSpellingLineNumberBegin != declExpansionLineNumberBegin)
            {
                const clang::SourceManager& sourceManager = context.getSourceManager();
                const clang::FileID fileId = sourceManager.getFileID(getSpellingSourceLocation(decl.getBeginLoc(), context));
                
                for (std::uint32_t i = declSpellingLineNumberBegin; i >= 0; --i)
                {
                    if (i == 0) return false;

                    const clang::SourceLocation beginOfLine = sourceManager.translateLineCol(fileId, i, 1);
                    const clang::SourceLocation endOfLine = sourceManager.translateLineCol(fileId, i + 1, 1).getLocWithOffset(-1);
                    const std::string line = dumpSourceRangeToString(clang::SourceRange(beginOfLine, endOfLine), sourceManager);

                    if (line.find(std::string("#define")) != std::string::npos) return true;
                }
            }

            return false;
        }

        static std::string getNameBeforeMacroExpansion(const clang::NamedDecl& decl, const std::shared_ptr<clang::Preprocessor>& preprocessor)
        {
            const std::string name = decl.getNameAsString();

            if (isThisAMacroExpansion(decl) && preprocessor.get())
            {
                const std::string nameBeforeMacroExpansion = preprocessor->getImmediateMacroName(decl.getLocation());
                if (nameBeforeMacroExpansion != std::string("")) return nameBeforeMacroExpansion;
            }
            
            return name;
        }

        struct Indentation
        {
            const std::uint32_t value;
            const std::uint32_t increment;

            Indentation(const std::uint32_t value, const std::uint32_t increment = 1)
                :
                value(value),
                increment(increment)
            { ; }

            Indentation(const clang::Decl& decl, const std::uint32_t increment = 0)
                :
                value(decl.getASTContext().getFullLoc(decl.getSourceRange().getBegin()).getSpellingColumnNumber() - 1),
                increment(increment > 0 ? increment : decl.getASTContext().getPrintingPolicy().Indentation)
            { ; }

            Indentation(const Indentation& indentation)
                :
                value(indentation.value),
                increment(indentation.increment)
            { ; }
        };

        Indentation operator+(const Indentation& indent, const std::uint32_t n)
        {
            return Indentation(indent.value + n * indent.increment, indent.increment);
        }

        Indentation operator-(const Indentation& indent, const std::uint32_t n)
        {
            if (indent.value < (n * indent.increment)) 
            {
                return Indentation(0, indent.increment);
            }
            else
            {
                return Indentation(indent.value - n * indent.increment, indent.increment);
            }
        }
    }
}

#endif