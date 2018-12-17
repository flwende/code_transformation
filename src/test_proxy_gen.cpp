// Copyright (c) 2017-2018 Florian Wende (flwende@gmail.com)
//
// Distributed under the BSD 2-clause Software License
// (See accompanying file LICENSE)

#include <trafo/data_layout/proxy_gen.hpp>

int main(int argc, const char **argv)
{
    using namespace llvm::cl;
    using namespace clang::tooling;
    using namespace TRAFO_NAMESPACE;

    OptionCategory optionCategory("proxy_gen");
	CommonOptionsParser parser(argc, argv, optionCategory);
	ClangTool clangTool(parser.getCompilations(), parser.getSourcePathList());
	return clangTool.run(newFrontendActionFactory<InsertProxyClass>().get());
}