// Copyright (c) 2017-2018 Florian Wende (flwende@gmail.com)
//
// Distributed under the BSD 2-clause Software License
// (See accompanying file LICENSE)

#if !defined(CLASS_C_HPP)
#define CLASS_C_HPP

#include <iostream>
#include <cstdint>

namespace fw
{
    struct C;

    struct C
    {
    private:
        std::size_t dummy;
    public:
        C() : x(0) { ; }
        C(const double value) : x(value) { ; }
        C(const C& rhs) : x(rhs.x) { ; }

        C& operator=(const double value)
        {
            x = value;
            return *this;    
        }

        double x;
    };

    static std::ostream& operator<<(std::ostream& os, const C& c)
    {
        os << "(" << c.x << ")";
        return os;
    }

    struct D
    {
        // empty
    };
}

#endif
