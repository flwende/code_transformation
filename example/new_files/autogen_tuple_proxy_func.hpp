namespace fw
{
    template <typename T_1, typename T_2, typename T_3>
    inline fw::tuple<T_1, T_2, T_3> log(const fw::proxy_internal::tuple_proxy<T_1, T_2, T_3>& x)
    {
        return {std::log(x.x), std::log(x.y), std::log(x.z)};   
    }
}

namespace fw
{
    inline const fw::proxy_internal::tuple_proxy<int, int, float>& test(const fw::proxy_internal::tuple_proxy<int, int, float>& x)
    {
        return x;
    }
}

