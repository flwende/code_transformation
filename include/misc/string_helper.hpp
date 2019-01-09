// Copyright (c) 2017-2018 Florian Wende (flwende@gmail.com)
//
// Distributed under the BSD 2-clause Software License
// (See accompanying file LICENSE)

#if !defined(MISC_STRING_HELPER_HPP)
#define MISC_STRING_HELPER_HPP

#include <sstream>
#include <string>
#include <vector>
#include <clang/AST/AST.h>

#if !defined(TRAFO_NAMESPACE)
    #define TRAFO_NAMESPACE fw
#endif

namespace TRAFO_NAMESPACE
{
    namespace internal
    {
        static std::string dumpStmtToString(const clang::Stmt* const stmt, clang::SourceManager& sm)
        {
            if (stmt == nullptr) // return empty string if 'stmt' is invalid
            {
                return std::string("");
            }

            std::string str; // will hold the content
            llvm::raw_string_ostream buffer(str); // buffer using 'str' as internal storage
            stmt->dump(buffer, sm); // dump content of 'stmt' to 'buffer'
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

        static std::string dumpSourceRangeToString(const clang::SourceRange sourceRange, clang::SourceManager& sm, const clang::LangOptions& langOpts)
        {
            const llvm::StringRef sourceText = clang::Lexer::getSourceText(clang::CharSourceRange::getCharRange(sourceRange), sm, langOpts);
            return sourceText.str();
        }

        static std::string dumpSourceRangeToString(const clang::SourceRange sourceRange, clang::SourceManager& sm)
        {
            const clang::LangOptions langOpts;
            return dumpSourceRangeToString(sourceRange, sm, langOpts);
        }

        static std::vector<std::string> splitString(const std::string str, const char delimiter)
        {
            std::vector<std::string> output;
            std::string tmp;
            std::istringstream input(str);

            while (std::getline(input, tmp, delimiter))
            {
                if (tmp != "")
                {
                    output.push_back(tmp);
                }
            }

            return output;
        }
    }
}

#endif