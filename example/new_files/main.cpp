#include <iostream>
#include <vector>
#include <buffer/buffer.hpp>
#include "vec.hpp"
#include <buffer/buffer.hpp>
#include "tuple.hpp"

namespace fw
{
    template <typename T_1, typename T_2, typename T_3>
    inline fw::tuple<T_1, T_2, T_3> log(const fw::tuple<T_1, T_2, T_3>& x)
    {
        return {std::log(x.x), std::log(x.y), std::log(x.z)};   
    }

    inline const fw::tuple<int, int, float>& test(const fw::tuple<int, int, float>& x)
    {
        return x;
    }
}

int main(int argc, char** argv)
{
    using real_t = double;
    using vec_t = fw::vec<double, 3>;
    using tuple_t = fw::tuple<double, float, float>;

    vec_t x = 0.5;
    tuple_t y = 0.25f;

    std::cout << x << std::endl;
    std::cout << y << std::endl;

    const std::size_t n = (argc > 1 ? atoi(argv[1]) : 16);

    XXX_NAMESPACE::buffer<vec_t, 1, XXX_NAMESPACE::data_layout::SoA> field_1{{n}};
    XXX_NAMESPACE::buffer<vec_t, 2, XXX_NAMESPACE::data_layout::SoA> field_x{{5, 13}};
    XXX_NAMESPACE::buffer<vec_t, 2, XXX_NAMESPACE::data_layout::SoA> field_y{{34, 7}};
    //std::vector<tuple_t> field_2(n);
    XXX_NAMESPACE::buffer<tuple_t, 1, XXX_NAMESPACE::data_layout::SoA> field_2{{16}};
    XXX_NAMESPACE::buffer<tuple_t, 1, XXX_NAMESPACE::data_layout::SoA> field_3{{n}};

    srand48(1);
    for (std::size_t i = 0; i < n; ++i)
    {
        field_2[i] = 10.0 * drand48();
    }

    #pragma omp simd
    for (std::size_t i = 0; i < n; ++i)
    {
        field_3[i] = fw::log(field_2[i]);
    }

    for (std::size_t i = 0; i < n; ++i)
    {
        std::cout << field_2[i] << " -> " << field_3[i] << std::endl;
    }

    return 0;
}

