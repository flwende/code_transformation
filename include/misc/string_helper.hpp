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
            if (stmt == nullptr) // return empty string if 'stmt' is invalid
            {
                return std::string("");
            }

            std::string str; // will hold the content
            llvm::raw_string_ostream buffer(str); // buffer using 'str' as internal storage
            // note: 'const' reference is casted away (looks like the signature of 'dump' is inappropriate)!
            stmt->dump(buffer, const_cast<clang::SourceManager&>(sourceManager)); // dump content of 'stmt' to 'buffer'
            return buffer.str(); // get content of 'buffer'
        }

        static std::string dumpDeclToString(const clang::Decl* const decl)
        {
            if (decl == nullptr) // return empty string if 'decl' is invalid
            {
                return std::string("");
            }

            std::string str; // will hold the content
            llvm::raw_string_ostream buffer(str); // buffer using 'str' as internal storage
            decl->dump(buffer); // dump content of 'decl' to 'buffer'
            return buffer.str(); // get content of 'buffer'
        }

        static std::string dumpStmtToStringHumanReadable(const clang::Stmt* const stmt, const clang::LangOptions& langOpts, const bool insertLeadingNewline)
        {
            if (stmt == nullptr) // return empty string if 'stmt' is invalid
            {
                return std::string("");
            }

            std::string str; // will hold the content
            llvm::raw_string_ostream buffer(str); // buffer using 'str' as internal storage
            stmt->printPretty(buffer, nullptr, clang::PrintingPolicy(langOpts), 0); // dump content of 'stmt' to 'buffer'
            return (insertLeadingNewline ? std::string("\n") : std::string("")) + buffer.str(); // get content of 'buffer'
        }

        static std::string dumpStmtToStringHumanReadable(const clang::Stmt* const stmt, const bool insertLeadingNewline)
        {
            const clang::LangOptions langOpts;
            return dumpStmtToStringHumanReadable(stmt, langOpts, insertLeadingNewline);
        }

        static std::string dumpDeclToStringHumanReadable(const clang::Decl* const decl, const clang::LangOptions& langOpts, const bool insertLeadingNewline)
        {
            if (decl == nullptr) // return empty string if 'decl' is invalid
            {
                return std::string("");
            }

            std::string str; // will hold the content
            llvm::raw_string_ostream buffer(str); // buffer using 'str' as internal storage
            decl->print(buffer, clang::PrintingPolicy(langOpts), 0); // dump content of 'decl' to 'buffer'
            return (insertLeadingNewline ? std::string("\n") : std::string("")) + buffer.str(); // get content of 'buffer'
        }

        static std::string dumpDeclToStringHumanReadable(const clang::Decl* const decl, const bool insertLeadingNewline)
        {
            const clang::LangOptions langOpts;
            return dumpDeclToStringHumanReadable(decl, langOpts, insertLeadingNewline);
        }

        static std::string dumpSourceRangeToString(const clang::SourceRange sourceRange, const clang::SourceManager& sourceManager, const clang::LangOptions& langOpts)
        {
            if (!sourceRange.isValid()) return std::string("");

            const llvm::StringRef sourceText = clang::Lexer::getSourceText(clang::CharSourceRange::getCharRange(sourceRange), sourceManager, langOpts);
            return sourceText.str();
        }

        static std::string dumpSourceRangeToString(const clang::SourceRange sourceRange, const clang::SourceManager& sourceManager)
        {
            const clang::LangOptions langOpts;
            return dumpSourceRangeToString(sourceRange, sourceManager, langOpts);
        }

        static std::vector<std::string> splitString(const std::string& input, const char delimiter)
        {
            std::vector<std::string> output;
            std::string tmp;
            std::istringstream inputStream(input);
            while (std::getline(inputStream, tmp, delimiter))
            {
                if (tmp != "")
                {
                    output.push_back(tmp);
                }
            }
            return output;
        }

        static std::string removeSpaces(const std::string& str)
        {
            std::string output(str);
            output.erase(std::remove_if(output.begin(), output.end(), ::isspace), output.end());
            return output;
        }

        static std::string concat(const std::vector<std::string>& input, const std::string delimiter = "")
        {
            std::string output;
            const std::uint32_t numElements = input.size();
            std::uint32_t elementId = 0;
            for (auto element : input)
            {
                output += element + (++elementId < numElements ? delimiter : std::string(""));
            }
            return output;
        }
    }
}

#endif