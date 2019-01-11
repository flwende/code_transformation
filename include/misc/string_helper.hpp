// Copyright (c) 2017-2018 Florian Wende (flwende@gmail.com)
//
// Distributed under the BSD 2-clause Software License
// (See accompanying file LICENSE)

#if !defined(MISC_STRING_HELPER_HPP)
#define MISC_STRING_HELPER_HPP

#include <sstream>
#include <string>
#include <cctype>
#include <vector>
#include <algorithm>
#include <clang/AST/AST.h>

#if !defined(TRAFO_NAMESPACE)
    #define TRAFO_NAMESPACE fw
#endif

namespace TRAFO_NAMESPACE
{
    namespace internal
    {
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

        static std::vector<std::string> splitString(const std::string& input, const char delimiter)
        {
            std::vector<std::string> output;
            std::string tmp;
            std::istringstream inputStream(input);

            while (std::getline(inputStream, tmp, delimiter))
            {
                if (tmp != std::string(""))
                {
                    output.push_back(tmp);
                }
            }

            return output;
        }

        static std::string removeSpaces(const std::string& input)
        {
            std::string output(input);

            output.erase(std::remove_if(output.begin(), output.end(), ::isspace), output.end());

            return output;
        }

        static std::string concat(const std::vector<std::string>& input, const std::string delimiter = std::string(""))
        {
            std::string output;
            const std::uint32_t numElements = input.size();
            std::uint32_t elementId = 0;

            for (const std::string element : input)
            {
                output += element + (++elementId < numElements ? delimiter : std::string(""));
            }

            return output;
        }
    }
}

#endif