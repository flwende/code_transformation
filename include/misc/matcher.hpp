// Copyright (c) 2017-2018 Florian Wende (flwende@gmail.com)
//
// Distributed under the BSD 2-clause Software License
// (See accompanying file LICENSE)

#if !defined(MATCHER_HPP)
#define MATCHER_HPP

#include <vector>
#include <memory>
#include <clang/ASTMatchers/ASTMatchFinder.h>

#if !defined(TRAFO_NAMESPACE)
    #define TRAFO_NAMESPACE fw
#endif

namespace TRAFO_NAMESPACE
{
    class Matcher
    {
        using Kernel = std::function<void(const clang::ast_matchers::MatchFinder::MatchResult&)>;

        class Action : public clang::ast_matchers::MatchFinder::MatchCallback
        {
            const Kernel kernel;

        public:

            Action(const Kernel& kernel)
                :
                kernel(kernel)
            { ; }

            void virtual run(const clang::ast_matchers::MatchFinder::MatchResult& result)
            {
                kernel(result);
            }
        };

        std::unique_ptr<clang::ast_matchers::MatchFinder> matcher;   
        std::vector<std::unique_ptr<Action>> actions;
        
    public:

        Matcher()
            :
            matcher(nullptr)
        { ; }
        
        template <typename T>
        void add(const T& match, const Kernel& kernel)
        {
            if (matcher.get() == nullptr)
            {
                matcher = std::unique_ptr<clang::ast_matchers::MatchFinder>(new clang::ast_matchers::MatchFinder());
            }
             
            actions.emplace_back(new Action(kernel));
            matcher->addMatcher(match, actions.back().get());
        }

        void run(clang::ASTContext& context)
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