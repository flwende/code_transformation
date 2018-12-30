// Copyright (c) 2017-2018 Florian Wende (flwende@gmail.com)
//
// Distributed under the BSD 2-clause Software License
// (See accompanying file LICENSE)

#if !defined(CLASS_B_HPP)
#define CLASS_B_HPP

#include <iostream>
#include <cstdint>

namespace fw
{
    class B
    {
        std::size_t dummy;
    public:
        B() : v(0), w(0) { ; }
        B(const double value) : v(value), w(value) { ; }
        B(const B& rhs) : v(rhs.v), w(rhs.w) { ; }

        B& operator=(const double value)
        {
            v = value;
            w = value;
            return *this;    
        }

        double v;
        double w;
    };

    static std::ostream& operator<<(std::ostream& os, const B& b)
    {
        os << "(" << b.v << "," << b.w << ")";
        return os;
    }
}

#endif
