// Copyright (c) 2017-2018 Florian Wende (flwende@gmail.com)
//
// Distributed under the BSD 2-clause Software License
// (See accompanying file LICENSE)

#if !defined(CLASS_A_HPP)
#define CLASS_A_HPP

#include <iostream>
#include <cstdint>

using real_t = float;

#if !defined MY_NAMESPACE
    #define MY_NAMESPACE fw
#endif

namespace MY_NAMESPACE
{
    template <typename T, std::size_t D, typename TT>
    class A;

    template <typename T, typename TT>
    class A<T, 2, TT>
    {
        static constexpr std::size_t D = 2;
        using fundamental_type = T;
    public:
        A() : x(0), y(0) { ; }
        A(const T value) : x(value), y(value) { ; }
        A(const A& rhs) : x(rhs.x), y(rhs.y) { ; }

        A& operator=(const T value)
        {
            x = value;
            y = value;
            return *this;    
        }

        T x;
        T y;
    };

    template <typename T, typename TT>
    class A<T, 3, TT>
    {
        static_assert(!std::is_const<T>::value, "error: A<const T, 1, TT> is not allowed");

        using fundamental_type = T;
        static constexpr std::size_t D = 3;

    public:

        A() : x(0), y(0), z(0) { ; }
        A(const T c) : x(c), y(c), z(c) { ; }
        A(const T x, const T y, const T z) : x(x), y(y), z(z) { ; }
        template <typename X>
        A(const A<X, 3, TT>& rhs) : x(rhs.x), y(rhs.y), z(rhs.z) { ; }

        A& operator=(const T c)
        {
            x = c;
            y = c;
            z = c;
            return *this;    
        }

        template <typename X>
        A& operator=(const A<X, 3, TT>& rhs)
        {
            x = rhs.x;
            y = rhs.y;
            z = rhs.z;
            return *this;    
        }

        #define MACRO(OP, IN_T)                                 \
        template <typename X>                                   \
        inline void operator OP (const IN_T<X, 3, TT> & rhs)    \
        {                                                       \
            x OP rhs.x;                                         \
            y OP rhs.y;                                         \
            z OP rhs.z;                                         \
        }
        MACRO(+=, A)
        MACRO(-=, A)
        MACRO(*=, A)
        MACRO(/=, A)
        #undef MACRO

        #define MACRO(OP)                           \
        inline void operator OP (const T c)         \
        {                                           \
            x OP c;                                 \
            y OP c;                                 \
            z OP c;                                 \
        }
        MACRO(+=)
        MACRO(-=)
        MACRO(*=)
        MACRO(/=)
        #undef MACRO

        T x;
        T y;
        T z;

        template <typename X>
        const A& foo(const A<X, 3, TT>& a) const
        {
            return *this;
        }

        const A& bar(const A& a) const 
        {
            return *this;
        }
    };

    template <typename T, typename TT>
    static std::ostream& operator<<(std::ostream& os, const A<T, 3, TT>& a)
    {
        os << "(" << a.x << "," << a.y << "," << a.z << ")";
        return os;
    }
}

#endif
