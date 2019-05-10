// Copyright (c) 2017-2019 Florian Wende (flwende@gmail.com)
//
// Distributed under the BSD 2-clause Software License
// (See accompanying file LICENSE)

#if !defined(VEC_VEC_HPP)
#define VEC_VEC_HPP

#include <auxiliary/math.hpp>

#if !defined(XXX_NAMESPACE)
#define XXX_NAMESPACE fw
#endif

#if !defined(VEC_NAMESPACE)
#define VEC_NAMESPACE XXX_NAMESPACE
#endif

namespace VEC_NAMESPACE
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //! \brief A simple vector with D components of the same type
    //!
    //! \tparam T data type
    //! \tparam D dimension
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    namespace proxy_internal
    {
        template <typename T, std::size_t D>
        class vec_proxy;
    }

    template <typename T, std::size_t D>
    class vec;

    //! \brief D = 1 specialization with component x
    //!
    //! \tparam T data type
    template <typename T>
    class vec<T, 1>
    {
        static_assert(std::is_fundamental<T>::value, "error: T is not a fundamental data type");
        static_assert(!std::is_void<T>::value, "error: T is void -> not allowed");
        static_assert(!std::is_volatile<T>::value, "error: T is volatile -> not allowed");

    public:

        //! Remember the template type parameter T
        using value_type = T;
        //! Remember the template parameter D (=1)
        static constexpr std::size_t d = 1;

        T x;

        //! Constructors
        vec(const T x = 0)
            :
            x(x) {}

        template <typename X>
        vec(const vec<X, 1>& v)
            :
            x(v.x) {}
        
        //! Some operators
        inline vec operator-() const
        {
            return vec(-x);
        }

    #define MACRO(OP, IN_T)                             \
        template <typename X>                           \
        inline vec& operator OP (const IN_T<X, 1>& v)   \
        {                                               \
            x OP v.x;                                   \
            return *this;                               \
        }                                               \
        
        MACRO(=, vec)
        MACRO(+=, vec)
        MACRO(-=, vec)
        MACRO(*=, vec)
        MACRO(/=, vec)

    #undef MACRO

    #define MACRO(OP)                                   \
        template <typename X>                           \
        inline vec& operator OP (const X x)             \
        {                                               \
            x OP x;                                     \
            return *this;                               \
        }                                               \

        MACRO(=)
        MACRO(+=)
        MACRO(-=)
        MACRO(*=)
        MACRO(/=)

    #undef MACRO

        //! \brief Return the Euclidean norm of the vector
        //!
        //! \return Euclidean norm
        inline T length() const
        {
            return std::abs(x);
        }
    };

    //! \brief D = 2 specialization with component x
    //!
    //! \tparam T data type
    template <typename T>
    class vec<T, 2>
    {
        static_assert(std::is_fundamental<T>::value, "error: T is not a fundamental data type");
        static_assert(!std::is_void<T>::value, "error: T is void -> not allowed");
        static_assert(!std::is_volatile<T>::value, "error: T is volatile -> not allowed");

    public:

        //! Remember the template type parameter T
        using value_type = T;
        //! Remember the template parameter D (=2)
        static constexpr std::size_t d = 2;

        T x;
        T y;

        //! Constructors
        vec(const T xy = 0) 
            :
            x(xy),
            y(xy) {}

        vec(const T x, const T y)
            :
            x(x),
            y(y) {}

        template <typename X>
        vec(const vec<X, 2>& v)
            :
            x(v.x),
            y(v.y) {}
        
        //! Some operators
        inline vec operator-() const
        {
            return vec(-x, -y);
        }

    #define MACRO(OP, IN_T)                             \
        template <typename X>                           \
        inline vec& operator OP (const IN_T<X, 2>& v)   \
        {                                               \
            x OP v.x;                                   \
            y OP v.y;                                   \
            return *this;                               \
        }                                               \

        MACRO(=, vec)
        MACRO(+=, vec)
        MACRO(-=, vec)
        MACRO(*=, vec)
        MACRO(/=, vec)

    #undef MACRO

    #define MACRO(OP)                                   \
        template <typename X>                           \
        inline vec& operator OP (const X xy)            \
        {                                               \
            x OP xy;                                    \
            y OP xy;                                    \
            return *this;                               \
        }                                               \

        MACRO(=)
        MACRO(+=)
        MACRO(-=)
        MACRO(*=)
        MACRO(/=)

    #undef MACRO

        //! \brief Return the Euclidean norm of the vector
        //!
        //! \return Euclidean norm
        inline T length() const
        {
            return MATH_NAMESPACE::math<T>::sqrt(x * x + y * y);
        }
    };
    
    //! \brief D = 3 specialization with component x
    //!
    //! \tparam T data type
    template <typename T>
    class vec<T, 3>
    {
        static_assert(std::is_fundamental<T>::value, "error: T is not a fundamental data type");
        static_assert(!std::is_void<T>::value, "error: T is void -> not allowed");
        static_assert(!std::is_volatile<T>::value, "error: T is volatile -> not allowed");

    public:
        using type = vec<T, 3>;

        using proxy_type = typename fw::proxy_internal::vec_proxy<T, 3>;

        //! Remember the template type parameter T
        using value_type = T;
        //! Remember the template parameter D (=3)
        static constexpr std::size_t d = 3;

        T x;
        T y;
        T z;

        //! Constructors
        vec(const T xyz = 0)
            :
            x(xyz),
            y(xyz),
            z(xyz) {}

        vec(const T x, const T y, const T z)
            :
            x(x),
            y(y),
            z(z) {}

        template <typename X>
        vec(const vec<X, 3>& v)
            :
            x(v.x),
            y(v.y),
            z(v.z) {}
        template <typename X>
        vec(const proxy_internal::vec_proxy<X, 3>& v)
            :
            x(v.x),
            y(v.y),
            z(v.z) {}
      
        //! Some operators
        inline vec operator-() const
        {
            return vec(-x, -y, -z);
        }

    #define MACRO(OP, IN_T)                             \
        template <typename X>                           \
        inline vec& operator OP (const IN_T<X, 3>& v)   \
        {                                               \
            x OP v.x;                                   \
            y OP v.y;                                   \
            z OP v.z;                                   \
            return *this;                               \
        }                                               \
        
        MACRO(=, vec)
        MACRO(=, proxy_internal::vec_proxy)
        MACRO(+=, vec)
        MACRO(+=, proxy_internal::vec_proxy)
        MACRO(-=, vec)
        MACRO(-=, proxy_internal::vec_proxy)
        MACRO(*=, vec)
        MACRO(*=, proxy_internal::vec_proxy)
        MACRO(/=, vec)
        MACRO(/=, proxy_internal::vec_proxy)

    #undef MACRO

    #define MACRO(OP)                                   \
        template <typename X>                           \
        inline vec& operator OP (const X xyz)           \
        {                                               \
            x OP xyz;                                   \
            y OP xyz;                                   \
            z OP xyz;                                   \
            return *this;                               \
        }                                               \

        MACRO(=)
        MACRO(+=)
        MACRO(-=)
        MACRO(*=)
        MACRO(/=)

    #undef MACRO

        //! \brief Return the Euclidean norm of the vector
        //!
        //! \return Euclidean norm
        inline T length() const
        {
            return MATH_NAMESPACE::math<T>::sqrt(x * x + y * y + z * z);
        }
    };

    template <typename T>
    std::ostream& operator<<(std::ostream& os, const vec<T, 1>& v)
    {
        os << "(" << v.x << ")";
        return os;
    }

    template <typename T>
    std::ostream& operator<<(std::ostream& os, const vec<T, 2>& v)
    {
        os << "(" << v.x << "," << v.y << ")";
        return os;
    }

    template <typename T>
    std::ostream& operator<<(std::ostream& os, const vec<T, 3>& v)
    {
        os << "(" << v.x << "," << v.y << "," << v.z << ")";
        return os;
    }
}

#include "autogen_vec_proxy.hpp"

#include <common/traits.hpp>

namespace XXX_NAMESPACE
{
    namespace internal
    {
        template <typename T, std::size_t D>
        struct provides_proxy_type<fw::vec<T, D>>
        {
            static constexpr bool value = true;
        };
    }
}

namespace XXX_NAMESPACE
{
    namespace internal
    {
        template <typename T, std::size_t D>
        struct provides_proxy_type<const fw::vec<T, D>>
        {
            static constexpr bool value = true;
        };
    }
}

#endif
