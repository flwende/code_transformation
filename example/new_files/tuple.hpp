// Copyright (c) 2017-2019 Florian Wende (flwende@gmail.com)
//
// Distributed under the BSD 2-clause Software License
// (See accompanying file LICENSE)

#if !defined(TUPLE_TUPLE_HPP)
#define TUPLE_TUPLE_HPP

#include <auxiliary/math.hpp>

#if !defined(XXX_NAMESPACE)
#define XXX_NAMESPACE fw
#endif

#if !defined(TUPLE_NAMESPACE)
#define TUPLE_NAMESPACE XXX_NAMESPACE
#endif

namespace TUPLE_NAMESPACE
{
    // some forward declarations
    namespace internal
    {
        template <typename T_1, typename T_2, typename T_3>
        class tuple_proxy;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //! \brief A simple tuple with 3 components of different type
    //!
    //! \tparam T_1 data type
    //! \tparam T_2 data type
    //! \tparam T_3 data type
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    namespace proxy_internal
    {
        template <typename T_1, typename T_2, typename T_3>
        class tuple_proxy;
    }

    template <typename T_1, typename T_2, typename T_3>
    class tuple
    {
        static_assert(std::is_fundamental<T_1>::value, "error: T_1 is not a fundamental data type");
        static_assert(!std::is_void<T_1>::value, "error: T_1 is void -> not allowed");
        static_assert(!std::is_volatile<T_1>::value, "error: T_1 is volatile -> not allowed");
        static_assert(!std::is_const<T_1>::value, "error: T_1 is const -> not allowed");

        static_assert(std::is_fundamental<T_2>::value, "error: T_2 is not a fundamental data type");
        static_assert(!std::is_void<T_2>::value, "error: T_2 is void -> not allowed");
        static_assert(!std::is_volatile<T_2>::value, "error: T_2 is volatile -> not allowed");
        static_assert(!std::is_const<T_2>::value, "error: T_2 is const -> not allowed");

        static_assert(std::is_fundamental<T_3>::value, "error: T_3 is not a fundamental data type");
        static_assert(!std::is_void<T_3>::value, "error: T_3 is void -> not allowed");
        static_assert(!std::is_volatile<T_3>::value, "error: T_3 is volatile -> not allowed");
        static_assert(!std::is_const<T_3>::value, "error: T_3 is const -> not allowed");

    public:
        using type = tuple<T_1, T_2, T_3>;

        using proxy_type = typename fw::proxy_internal::tuple_proxy<T_1, T_2, T_3>;

        using type = tuple<T_1, T_2, T_3>;
        using proxy_type = typename internal::tuple_proxy<T_1, T_2, T_3>;
        using value_type = typename XXX_NAMESPACE::internal::compare<T_1, typename XXX_NAMESPACE::internal::compare<T_2, T_3>::stronger_type>::stronger_type;

        T_1 x;
        T_2 y;
        T_3 z;

        tuple() : x(0), y(0), z(0) {}

        template <typename X>
        tuple(const X xyz) : x(xyz), y(xyz), z(xyz) {}
        tuple(const T_1 x, const T_2 y, const T_3 z) : x(x), y(y), z(z) {}

        template <typename X_1, typename X_2, typename X_3>
        tuple(const tuple<X_1, X_2, X_3>& t) : x(t.x), y(t.y), z(t.z) {}
        template <typename X_1, typename X_2, typename X_3>
        tuple(const proxy_internal::tuple_proxy<X_1, X_2, X_3>& t) : x(t.x), y(t.y), z(t.z) {}
        
        template <typename X_1, typename X_2, typename X_3>
        tuple(const internal::tuple_proxy<X_1, X_2, X_3>& tp) : x(tp.x), y(tp.y), z(tp.z) {}

        //! Some operators
        inline tuple operator-() const
        {
            return tuple(-x, -y, -z);
        }

    #define MACRO(OP, IN_T)                                         \
        template <typename X_1, typename X_2, typename X_3>         \
        inline tuple& operator OP (const IN_T<X_1, X_2, X_3>& t)    \
        {                                                           \
            x OP t.x;                                               \
            y OP t.y;                                               \
            z OP t.z;                                               \
            return *this;                                           \
        }                                                           \
        
        MACRO(=, tuple)
        MACRO(=, proxy_internal::tuple_proxy)
        MACRO(+=, tuple)
        MACRO(+=, proxy_internal::tuple_proxy)
        MACRO(-=, tuple)
        MACRO(-=, proxy_internal::tuple_proxy)
        MACRO(*=, tuple)
        MACRO(*=, proxy_internal::tuple_proxy)
        MACRO(/=, tuple)
        MACRO(/=, proxy_internal::tuple_proxy)

        MACRO(=, internal::tuple_proxy)
        MACRO(+=, internal::tuple_proxy)
        MACRO(-=, internal::tuple_proxy)
        MACRO(*=, internal::tuple_proxy)
        MACRO(/=, internal::tuple_proxy)

    #undef MACRO

    #define MACRO(OP)                                               \
        template <typename X>                                       \
        inline tuple& operator OP (const X xyz)                     \
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

    template <typename T_1, typename T_2, typename T_3>
    std::ostream& operator<<(std::ostream& os, const tuple<T_1, T_2, T_3>& v)
    {
        os << "(" << v.x << "," << v.y << "," << v.z << ")";
        return os;
    }
}

#include "autogen_tuple_proxy.hpp"

#include <common/traits.hpp>

namespace XXX_NAMESPACE
{
    namespace internal
    {
        template <typename T_1, typename T_2, typename T_3>
        struct provides_proxy_type<fw::tuple<T_1, T_2, T_3>>
        {
            static constexpr bool value = true;
        };
    }
}

namespace XXX_NAMESPACE
{
    namespace internal
    {
        template <typename T_1, typename T_2, typename T_3>
        struct provides_proxy_type<const fw::tuple<T_1, T_2, T_3>>
        {
            static constexpr bool value = true;
        };
    }
}

#endif