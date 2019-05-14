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
    private:

        template <typename _X, std::size_t _N, std::size_t _D, XXX_NAMESPACE::data_layout _L>
        friend class XXX_NAMESPACE::internal::accessor;

        template <typename _P, std::size_t _R>
        friend class XXX_NAMESPACE::internal::iterator;

        using T_unqualified = typename std::remove_cv<T>::type;

        static constexpr bool is_const_type = (std::is_const<T>::value);

    public:

        using type = vec_proxy<T, 3>;

        using const_type = vec_proxy<const T, 3>;

        using base_pointer = XXX_NAMESPACE::multi_pointer_n<T, 3>;

        using original_type = typename std::conditional<is_const_type,
            const fw::vec<T_unqualified, 3>,
            fw::vec<T_unqualified, 3>>::type;

    private:

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

    namespace proxy_internal
    {
    template <typename T>
    std::ostream& operator<<(std::ostream& os, const vec_proxy<T, 1>& v)
    {
        os << "(" << v.x << ")";
        return os;
    }
    }
    namespace proxy_internal
    {
    template <typename T>
    std::ostream& operator<<(std::ostream& os, const vec_proxy<T, 2>& v)
    {
        os << "(" << v.x << "," << v.y << ")";
        return os;
    }
    }
    namespace proxy_internal
    {
    template <typename T>
    std::ostream& operator<<(std::ostream& os, const vec_proxy<T, 3>& v)
    {
        os << "(" << v.x << "," << v.y << "," << v.z << ")";
        return os;
    }
    }
}
