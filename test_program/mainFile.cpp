// Copyright (c) 2017-2018 Florian Wende (flwende@gmail.com)
//
// Distributed under the BSD 2-clause Software License
// (See accompanying file LICENSE)

#include <iostream>
#include <iomanip>
#include <cstdint>
#include <cmath>
#include <vector>
#include "classA.hpp"
#include "classB.hpp"
#include "prefix_sum.hpp"

static std::vector<float> f;

int main(int argc, char** argv)
{
    std::size_t n = (argc > 1 ? atoi(argv[1]) : 16);
    std::vector<fw::A<real_t, 3, float>> x(n);
    std::vector<std::vector<fw::A<real_t, 3, float>>> y(1);
    std::vector<fw::extra::B<real_t>> ps(n);
    y[0].resize(n);

    srand48(1);
    for (std::size_t i = 0; i < n; ++i)
    {
        x[i] = drand48();
        y[0][i].x = std::sqrt(x[i].y);
    }

    ps = prefix_sum(x, 'y');

    std::cout << std::fixed << std::setprecision(4);
    for (std::size_t i = 0; i < n; ++i)
    {
        std::cout << x[i] << ", " << y[0][i] << ", " << ps[i] << std::endl;
    }

    return 0;
}
