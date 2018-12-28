// Copyright (c) 2017-2018 Florian Wende (flwende@gmail.com)
//
// Distributed under the BSD 2-clause Software License
// (See accompanying file LICENSE)

#if !defined(REWRITER_HPP)
#define REWRITER_HPP

#include <vector>
#include <memory>
#include <clang/ASTMatchers/ASTMatchFinder.h>

#if !defined(TRAFO_NAMESPACE)
    #define TRAFO_NAMESPACE fw
#endif

namespace TRAFO_NAMESPACE
{
    class Rewriter
    {
        using Kernel = std::function<void(const clang::ast_matchers::MatchFinder::MatchResult&, Rewriter&)>;

        class Action : public clang::ast_matchers::MatchFinder::MatchCallback
        {
            const Kernel kernel;
            Rewriter& rewriter;

        public:

            Action(const Kernel& kernel, Rewriter& rewriter)
                :
                kernel(kernel),
                rewriter(rewriter)
            { ; }

            void virtual run(const clang::ast_matchers::MatchFinder::MatchResult& result)
            {
                kernel(result, rewriter);
            }
        };

        std::unique_ptr<clang::ast_matchers::MatchFinder> matcher;   
        std::vector<std::unique_ptr<Action>> actions;
        clang::Rewriter& rewriter;
        
    public:

        Rewriter(clang::Rewriter& rewriter)
            :
            matcher(nullptr),
            rewriter(rewriter)
        { ; }

        const clang::LangOptions& getLangOpts() const
        {
            return rewriter.getLangOpts();
        }

        bool insert(const clang::SourceLocation& sourceLocation, const std::string& text, const bool insertAfter = true, const bool indentNewLines = false)
        {
            return rewriter.InsertText(sourceLocation, text, insertAfter, indentNewLines);
        }

        bool replace(const clang::SourceRange& sourceRange, const std::string& text)
        {
            return rewriter.ReplaceText(sourceRange, text);
        }
        
        bool InsertText(const clang::SourceLocation& sourceLocation, const std::string& text, const bool insertAfter = true, const bool indentNewLines = false)
        {
            // compatibility with clang::Rewriter interface 
            return insert(sourceLocation, text, insertAfter, indentNewLines);
        }

        bool ReplaceText(const clang::SourceRange& sourceRange, const std::string& text)
        {
            // compatibility with clang::Rewriter interface
            return replace(sourceRange, text);
        }

        template <typename T>
        void add(const T& match, const Kernel& kernel)
        {
            if (matcher.get() == nullptr)
            {
                matcher = std::unique_ptr<clang::ast_matchers::MatchFinder>(new clang::ast_matchers::MatchFinder());
            }

            actions.emplace_back(new Action(kernel, *this));
            matcher->addMatcher(match, actions.back().get());
        }

        void run(clang::ASTContext& context) const
        {
            if (matcher.get() != nullptr)
            {
                matcher->matchAST(context);
            }
        }

        void clear()
        {
            if (matcher.get() != nullptr)
            {
                delete matcher.release();
            }
            
            actions.clear();
        }
    };
}

#endif