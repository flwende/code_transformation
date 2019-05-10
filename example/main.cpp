#include <iostream>
#include <vector>
#include <vec/vec.hpp>
#include <tuple/tuple.hpp>

int main()
{
    using real_t = double;
    using vec_t = fw::vec<double, 3>;
    using tuple_t = fw::tuple<double, float, float>;

    vec_t x = 0.5;
    tuple_t y = 0.25f;

    std::cout << x << std::endl;
    std::cout << y << std::endl;

    std::vector<vec_t> field_1(12);
    std::vector<tuple_t> field_2(13);

    return 0;
}
