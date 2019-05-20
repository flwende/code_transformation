// my header

#include <common/memory.hpp>
#include <common/data_layout.hpp>

namespace XXX_NAMESPACE
{
    namespace internal 
    {
        template <typename P, typename R>
        class iterator;
    }
}

namespace XXX_NAMESPACE
{
    namespace internal 
    {
        template <typename X, std::size_t N, std::size_t D, XXX_NAMESPACE::data_layout L>
        class accessor;
    }
}

namespace TUPLE_NAMESPACE
{
    namespace proxy_internal
    {
    template <typename T_1, typename T_2, typename T_3>
    class tuple_proxy
    {
    private:

        template <typename _X, std::size_t _N, std::size_t _D, XXX_NAMESPACE::data_layout _L>
        friend class XXX_NAMESPACE::internal::accessor;

        template <typename _P, std::size_t _R>
        friend class XXX_NAMESPACE::internal::iterator;

        using T_1_unqualified = typename std::remove_cv<T_1>::type;

        using T_2_unqualified = typename std::remove_cv<T_2>::type;

        using T_3_unqualified = typename std::remove_cv<T_3>::type;

        static constexpr bool is_const_type = (std::is_const<T_1>::value || std::is_const<T_2>::value || std::is_const<T_3>::value);

    public:

        using type = tuple_proxy<T_1, T_2, T_3>;

        using const_type = tuple_proxy<const T_1, const T_2, const T_3>;

        using base_pointer = XXX_NAMESPACE::multi_pointer_inhomogeneous<T_1, T_2, T_3>;

        using original_type = typename std::conditional<is_const_type,
            const fw::tuple<T_1_unqualified, T_2_unqualified, T_3_unqualified>,
            fw::tuple<T_1_unqualified, T_2_unqualified, T_3_unqualified>>::type;

    private:

        static_assert(std::is_fundamental<T_1>::value, "error: T_1 is not a fundamental data type");
        static_assert(!std::is_void<T_1>::value, "error: T_1 is void -> not allowed");
        static_assert(!std::is_volatile<T_1>::value, "error: T_1 is volatile -> not allowed");

        static_assert(std::is_fundamental<T_2>::value, "error: T_2 is not a fundamental data type");
        static_assert(!std::is_void<T_2>::value, "error: T_2 is void -> not allowed");
        static_assert(!std::is_volatile<T_2>::value, "error: T_2 is volatile -> not allowed");

        static_assert(std::is_fundamental<T_3>::value, "error: T_3 is not a fundamental data type");
        static_assert(!std::is_void<T_3>::value, "error: T_3 is void -> not allowed");
        static_assert(!std::is_volatile<T_3>::value, "error: T_3 is volatile -> not allowed");

    public:

        T_1& x;
        T_2& y;
        T_3& z;

        tuple_proxy(base_pointer base)
            :
            x(*(std::get<0>(base.ptr))),
            y(*(std::get<1>(base.ptr))),
            z(*(std::get<2>(base.ptr)))
        {}


        tuple_proxy(std::tuple<T_1&, T_2&, T_3&> obj)
            :
            x(std::get<0>(obj)),
            y(std::get<1>(obj)),
            z(std::get<2>(obj))
        {}

        

        
        
        //! Some operators
        inline fw::tuple<T_1_unqualified, T_2_unqualified, T_3_unqualified> operator-() const
        {
            return fw::tuple<T_1_unqualified, T_2_unqualified, T_3_unqualified>(-x, -y, -z);
        }

    #define MACRO(OP, IN_T)                                         \
        template <typename X_1, typename X_2, typename X_3>         \
        inline tuple_proxy& operator OP (const IN_T<X_1, X_2, X_3>& t)    \
        {                                                           \
            x OP t.x;                                               \
            y OP t.y;                                               \
            z OP t.z;                                               \
            return *this;                                           \
        }                                                           \
        
        MACRO(=, fw::tuple)
        MACRO(=, tuple_proxy)
        MACRO(+=, fw::tuple)
        MACRO(+=, tuple_proxy)
        MACRO(-=, fw::tuple)
        MACRO(-=, tuple_proxy)
        MACRO(*=, fw::tuple)
        MACRO(*=, tuple_proxy)
        MACRO(/=, fw::tuple)
        MACRO(/=, tuple_proxy)

    #undef MACRO

    #define MACRO(OP)                                               \
        template <typename X>                                       \
        inline tuple_proxy& operator OP (const X xyz)                     \
        {                                                           \
            x OP xyz;                                               \
            y OP xyz;                                               \
            z OP xyz;                                               \
            return *this;                                           \
        }                                                           \

        MACRO(=)
        MACRO(+=)
        MACRO(-=)
        MACRO(*=)
        MACRO(/=)

    #undef MACRO
    };    
    }

    namespace proxy_internal
    {
    template <typename T_1, typename T_2, typename T_3>
    std::ostream& operator<<(std::ostream& os, const tuple_proxy<T_1, T_2, T_3>& v)
    {
        os << "(" << v.x << "," << v.y << "," << v.z << ")";
        return os;
    }
    }
}

#include "autogen_tuple_proxy_func.hpp"
