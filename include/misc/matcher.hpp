// Copyright (c) 2017-2019 Florian Wende (flwende@gmail.com)
//
// Distributed under the BSD 2-clause Software License
// (See accompanying file LICENSE)

#if !defined(MISC_MATCHER_HPP)
#define MISC_MATCHER_HPP

#include <memory>
#include <vector>
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

        template <typename T>
        void addMatcher(const T& match, const Kernel& kernel, const clang::NamedDecl* const root = nullptr)
        {
            using namespace clang::ast_matchers;
        
            if (!matcher.get())
            {
                matcher = std::unique_ptr<clang::ast_matchers::MatchFinder>(new clang::ast_matchers::MatchFinder());
            }

            if (root)
            {
                const auto wrapperKernel = [&kernel, &root] (const MatchFinder::MatchResult& result) mutable
                    {
                        if (const clang::NamedDecl* const decl = result.Nodes.getNodeAs<clang::NamedDecl>("rootDecl"))
                        {
                            if (decl == root)
                            {
                                kernel(result);
                            }
                        }
                    };
                    
                actions.emplace_back(new Action(wrapperKernel));
                matcher->addMatcher(decl(allOf(match, hasParent(namedDecl(hasName(root->getNameAsString())).bind("rootDecl")))), actions.back().get());
            }
            else
            {
                actions.emplace_back(new Action(kernel));
                matcher->addMatcher(match, actions.back().get());
            }
        }
        
        void run(clang::ASTContext& context)
        {
            if (matcher.get())
            {
                matcher->matchAST(context);
            }
        }

        void clear()
        {
            if (matcher.get())
            {
                delete matcher.release();
            }

            actions.clear();
        }
        
        template <typename N, typename T>
        static bool testDecl(const N& node, const T& match)
        {
            return test<clang::Decl>(node, match, node.getASTContext());
        }

    private:

        template <typename D, typename N, typename T>
        static bool test(const N& node, const T& match, clang::ASTContext& context)
        {
            using namespace clang::ast_matchers;

            MatchFinder matcher;
            bool testResult = false;

            Action tester([&testResult] (const MatchFinder::MatchResult& result)
                {
                    if (const D* const decl = result.Nodes.getNodeAs<D>("test"))
                    {        
                        testResult = true;
                    }
                });

            matcher.addMatcher(match.bind("test"), &tester);
            matcher.match(node, context);

            return testResult;
        }
    };
}

#endif