// Copyright (c) 2017-2018 Florian Wende (flwende@gmail.com)
//
// Distributed under the BSD 2-clause Software License
// (See accompanying file LICENSE)

#include "prefix_sum.hpp"
#include "classB.hpp"

using namespace fw;

std::vector<B> prefix_sum(const std::vector<A<real_t, 3>>& x, const char component)
{
    std::vector<B> ps;
    ps.reserve(x.size());

    ps[0].w = 0; // exclusive prefix sum
    if (component == 'x')
    {
        ps[0].v = x[0].x; // inclusive prefix sum
        for (std::size_t i = 1; i < x.size(); ++i)
        {
            ps[i].w = ps[i - 1].w + x[i - 1].x;
            ps[i].v = ps[i - 1].v + x[i].x;
        }
    }
    else if (component == 'y')
    {
        ps[0].v = x[0].y; // inclusive prefix sum
        for (std::size_t i = 1; i < x.size(); ++i)
        {
            ps[i].w = ps[i - 1].w + x[i - 1].y;
            ps[i].v = ps[i - 1].v + x[i].y;
        }
    }
    else
    {
        ps[0].v = x[0].z; // inclusive prefix sum
        for (std::size_t i = 1; i < x.size(); ++i)
        {
            ps[i].w = ps[i - 1].w + x[i - 1].z;
            ps[i].v = ps[i - 1].v + x[i].z;
        }
    }

    return ps;
}
