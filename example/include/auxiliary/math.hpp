// Copyright (c) 2017-2019 Florian Wende (flwende@gmail.com)
//
// Distributed under the BSD 2-clause Software License
// (See accompanying file LICENSE)

#if !defined(AUXILIARY_MATH_HPP)
#define AUXILIARY_MATH_HPP

#include <cmath>
#include <cstdint>
#include <type_traits>

#if !defined(XXX_NAMESPACE)
#define XXX_NAMESPACE fw
#endif

#if !defined(MATH_NAMESPACE)
#define MATH_NAMESPACE XXX_NAMESPACE
#endif

/*
#include "../common/traits.hpp"
#include "../sarray/sarray.hpp"
*/
namespace MATH_NAMESPACE
{
    /*
    ////////////////////////////////////////////////////////////////////////////////////////////////////////
    //! \brief Greatest common divisor (gcd)
    //! 
    //! \tparam T must be an unsigned integer
    //! \param x_1 input
    //! \param x_2 input
    //! \return gcd(x_1, x_2)
    ////////////////////////////////////////////////////////////////////////////////////////////////////////
    template <typename T>
    constexpr T greatest_common_divisor(T x_1, T x_2)
    {
        static_assert(std::is_integral<T>::value && std::is_unsigned<T>::value, "error: only unsigned integers allowed");

        while (true)
        {
            if (x_1 == 0) return x_2;
            x_2 %= x_1;
            if (x_2 == 0) return x_1;
            x_1 %= x_2;
        }

        return 0;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////
    //! \brief Least common multiple (lcm)
    //! 
    //! Note: it holds 'gcd(x_1, x_2) * lcm(x_1, x_2) = |x_1 * x_2|'
    //!
    //! \tparam T must be an unsigned integer
    //! \param x_1 input
    //! \param x_2 input
    //! \return lcm(x_1, x_2)
    ////////////////////////////////////////////////////////////////////////////////////////////////////////
    template <typename T>
    constexpr T least_common_multiple(T x_1, T x_2)
    {
        static_assert(std::is_integral<T>::value && std::is_unsigned<T>::value, "error: only unsigned integers allowed");

        return (x_1 * x_2) / greatest_common_divisor(x_1, x_2);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////
    //! \brief Power of N test
    //!
    //! \tparam N base
    //! \tparam T must be an unsigned integer
    //! \param x argument
    //! \return x is power of N or not
    ////////////////////////////////////////////////////////////////////////////////////////////////////////
    template <std::size_t N, typename T>
    constexpr bool is_power_of(T x)
    {
        static_assert(std::is_integral<T>::value && std::is_unsigned<T>::value, "error: only unsigned integers allowed");
        static_assert(N > 0, "error: N must be at least 1");

        if (x == 1 || x == N) return true;
        if (x <= 0 || x < N || N == 1) return false;

        while (true)
        {
            if (x == 0 || x % N) return false;
            x /= N;
            if (x == 1) return true;
        }

        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////
    //! \brief Prefix sum calculation
    //!
    //! \tparam T data type
    //! \tparam N array extent
    //! \param x argument
    //! \return prefix sum
    ////////////////////////////////////////////////////////////////////////////////////////////////////////
    template <typename T, std::size_t N>
    constexpr MATH_NAMESPACE::sarray<T, N> prefix_sum(const MATH_NAMESPACE::sarray<T, N>& x)
    {
        MATH_NAMESPACE::sarray<T, N> y{0};

        for (std::size_t i = 1; i < N; ++i)
        {
            y[i] = y[i - 1] + x[i - 1];
        }

        return y;
    }
*/
    ////////////////////////////////////////////////////////////////////////////////////////////////////////
    //! \brief Definition of some math functions and constants for different FP types
    ////////////////////////////////////////////////////////////////////////////////////////////////////////
    template <typename T>
    struct math
    {
        static constexpr T one = static_cast<T>(1.0);
        static constexpr T minus_one = static_cast<T>(-1.0);

        static T sqrt(const T x)
        {
            return std::sqrt(x);
        }

        static T log(const T x)
        {
            return std::log(x);
        }

        static T exp(const T x)
        {
            return std::exp(x);
        }
    };

    //! \brief Specialization with T = float
    template <>
    struct math<float>
    {
        static constexpr float one = 1.0F;
        static constexpr float minus_one = -1.0F;

        static float sqrt(const float x)
        {
            return sqrtf(x);
        }

        static float log(const float x)
        {
            return logf(x);
        }

        static float exp(const float x)
        {
            return expf(x);
        }
    };
}

#endif