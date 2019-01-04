// Copyright (c) 2017-2018 Florian Wende (flwende@gmail.com)
//
// Distributed under the BSD 2-clause Software License
// (See accompanying file LICENSE)

#include "prefix_sum.hpp"
#include "classB.hpp"
#include "classC.hpp"

using namespace fw;
using namespace fw::extra;

std::vector<B<real_t>> prefix_sum(const std::vector<A<real_t, 3>>& x, const char component)
{
    std::vector<B<real_t>> ps;
    ps.reserve(x.size());
    std::vector<C> ps_inclusive(x.size());
    std::vector<C> ps_exclusive(x.size());

    ps_exclusive[0].x = 0; // exclusive prefix sum
    if (component == 'x')
    {
        ps_inclusive[0] = x[0].x; // inclusive prefix sum
        for (std::size_t i = 1; i < x.size(); ++i)
        {
            ps_inclusive[i].x = ps_inclusive[i - 1].x + x[i].x;
            ps_exclusive[i].x = ps_exclusive[i - 1].x + x[i - 1].x;
        }
    }
    else if (component == 'y')
    {
        ps_inclusive[0].x = x[0].y; // inclusive prefix sum
        for (std::size_t i = 1; i < x.size(); ++i)
        {
            ps_inclusive[i].x = ps_inclusive[i - 1].x + x[i].y;
            ps_exclusive[i].x = ps_exclusive[i - 1].x + x[i - 1].y;
        }
    }
    else
    {
        ps_inclusive[0] = x[0].z; // inclusive prefix sum
        for (std::size_t i = 1; i < x.size(); ++i)
        {
            ps_inclusive[i].x = ps_inclusive[i - 1].x + x[i].z;
            ps_exclusive[i].x = ps_exclusive[i - 1].x + x[i - 1].z;
        }
    }

    for (std::size_t i = 0; i < x.size(); ++i)
    {
        ps[i].v = ps_inclusive[i].x;
        ps[i].w = ps_exclusive[i].x;
    }

    return ps;
}
