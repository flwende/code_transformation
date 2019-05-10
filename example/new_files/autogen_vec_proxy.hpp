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

namespace VEC_NAMESPACE
{
    namespace proxy_internal
    {
    template <typename T, std::size_t D>
    class vec_proxy;    
    }

    namespace proxy_internal
    {
    template <typename T>
    class vec_proxy<T, 3>
    {
        template <typename _X, std::size_t _N, std::size_t _D, data_layout _L>
        friend class XXX_NAMESPACE::internal::accessor;

        template <typename _P, std::size_t _R>
        friend class XXX_NAMESPACE::internal::iterator;

        using type = vec_proxy<T, 3>;

        using const_type = vec_proxy<const T, 3>;

        using T_unqualified = typename std::remove_cv<T>::type;

        using base_pointer = XXX_NAMESPACE::multi_pointer_n<T, 3>;

        static constexpr bool is_const_type = (std::is_const<T>::value);

        using original_type = typename std::conditional<is_const_type,
            const fw::vec<T_unqualified, 3>,
            fw::vec<T_unqualified, 3>>::type;

        static_assert(std::is_fundamental<T>::value, "error: T is not a fundamental data type");
        static_assert(!std::is_void<T>::value, "error: T is void -> not allowed");
        static_assert(!std::is_volatile<T>::value, "error: T is volatile -> not allowed");

    public:

        //! Remember the template type parameter T
        using value_type = T;
        //! Remember the template parameter D (=3)
        static constexpr std::size_t d = 3;

        T& x;
        T& y;
        T& z;

        //! Constructors
        vec_proxy(base_pointer base)
            :
            x(base.ptr[0 * base.n_0]),
            y(base.ptr[1 * base.n_0]),
            z(base.ptr[2 * base.n_0])
        {}


        vec_proxy(std::tuple<T&, T&, T&> obj)
            :
            x(std::get<0>(obj)),
            y(std::get<1>(obj)),
            z(std::get<2>(obj))
        {}


        
      
        //! Some operators
        inline fw::vec<T_unqualified, 3> operator-() const
        {
            return fw::vec<T_unqualified, 3>(-x, -y, -z);
        }

    #define MACRO(OP, IN_T)                             \
        template <typename X>                           \
        inline vec_proxy& operator OP (const IN_T<X, 3>& v)   \
        {                                               \
            x OP v.x;                                   \
            y OP v.y;                                   \
            z OP v.z;                                   \
            return *this;                               \
        }                                               \
        
        MACRO(=, fw::vec)
        MACRO(=, vec_proxy)
        MACRO(+=, fw::vec)
        MACRO(+=, vec_proxy)
        MACRO(-=, fw::vec)
        MACRO(-=, vec_proxy)
        MACRO(*=, fw::vec)
        MACRO(*=, vec_proxy)
        MACRO(/=, fw::vec)
        MACRO(/=, vec_proxy)

    #undef MACRO

    #define MACRO(OP)                                   \
        template <typename X>                           \
        inline vec_proxy& operator OP (const X xyz)           \
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
    }

}
