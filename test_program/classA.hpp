// Copyright (c) 2017-2018 Florian Wende (flwende@gmail.com)
//
// Distributed under the BSD 2-clause Software License
// (See accompanying file LICENSE)

#if !defined(CLASS_A_HPP)
#define CLASS_A_HPP

#include <iostream>
#include <cstdint>

using real_t = float;

namespace fw
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
        static constexpr std::size_t D = 3;
        using fundamental_type = T;
    public:
        A() : x(0), y(0), z(0) { ; }
        A(const T value) : x(value), y(value), z(value) { ; }
        A(const A& rhs) : x(rhs.x), y(rhs.y), z(rhs.z) { ; }

        A& operator=(const T value)
        {
            x = value;
            y = value;
            z = value;
            return *this;    
        }

        T x;
        T y;
        T z;
    };

    template <typename T, typename TT>
    static std::ostream& operator<<(std::ostream& os, const A<T, 3, TT>& a)
    {
        os << "(" << a.x << "," << a.y << "," << a.z << ")";
        return os;
    }
}

#endif
