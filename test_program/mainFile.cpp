// Copyright (c) 2017-2018 Florian Wende (flwende@gmail.com)
//
// Distributed under the BSD 2-clause Software License
// (See accompanying file LICENSE)

#include <iostream>
#include <cstdint>
#include <cmath>
#include <vector>
#include "classA.hpp"

using real_t = float;

int main(int argc, char** argv)
{
    std::size_t n = (argc > 1 ? atoi(argv[1]) : 16);
    std::vector<fw::A<real_t, 3>> x(n);
    std::vector<std::vector<fw::A<real_t, 3>>> y(1);
    y[0].resize(n);
    std::vector<float> f;

    srand48(1);
    for (std::size_t i = 0; i < n; ++i)
    {
        x[i] = 2.0F;
        y[0][i].x = std::sqrt(x[i].y);
    }

    for (std::size_t i = 0; i < n; ++i)
    {
        std::cout << x[i] << ", " << y[0][i] << std::endl;
    }

    return 0;
}
