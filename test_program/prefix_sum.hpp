// Copyright (c) 2017-2018 Florian Wende (flwende@gmail.com)
//
// Distributed under the BSD 2-clause Software License
// (See accompanying file LICENSE)

#if !defined(PREFIX_SUM_HPP)
#define PREFIX_SUM_HPP

#include <vector>
#include "classA.hpp"
#include "classB.hpp"

std::vector<fw::extra::B<real_t>> prefix_sum(const std::vector<fw::A<real_t, 3>>& x, const char component);

#endif