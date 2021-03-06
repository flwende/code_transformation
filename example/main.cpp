#include <iostream>
#include <vector>
#include <array>
#include <vec/vec.hpp>
#include <tuple/tuple.hpp>

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

    std::vector<vec_t> field_1(n);
    vec_t field_x[13][5];
    std::array<std::array<vec_t, 34>, 7> field_y;
    //std::vector<tuple_t> field_2(n);
    std::array<tuple_t, 16> field_2;
    std::vector<tuple_t> field_3(n);

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

