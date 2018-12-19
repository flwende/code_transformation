// Copyright (c) 2017-2018 Florian Wende (flwende@gmail.com)
//
// Distributed under the BSD 2-clause Software License
// (See accompanying file LICENSE)

#if !defined(REWRITER_HPP)
#define REWRITER_HPP

#include <string>
#include <clang/ASTMatchers/ASTMatchFinder.h>

#if !defined(TRAFO_NAMESPACE)
    #define TRAFO_NAMESPACE fw
#endif

namespace TRAFO_NAMESPACE
{
    class MyNewRewriter : public clang::ast_matchers::MatchFinder::MatchCallback
    {
        clang::Rewriter& rewriter;

    public:

        MyNewRewriter(clang::Rewriter& rewriter)
            :
            rewriter(rewriter)
        { ; }

        bool insertText(const clang::SourceLocation& sourceLocation, const std::string& text, const bool insertAfter = true, const bool indentNewLines = false) const
        {
            return rewriter.InsertText(sourceLocation, text, insertAfter, indentNewLines);
        }

        bool ReplaceText(const clang::SourceRange& sourceRange, const std::string& text) const
        {
            return rewriter.ReplaceText(sourceRange, text);
        }
    };
}

#endif