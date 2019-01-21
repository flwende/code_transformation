// Copyright (c) 2017-2018 Florian Wende (flwende@gmail.com)
//
// Distributed under the BSD 2-clause Software License
// (See accompanying file LICENSE)

#if !defined(MISC_STRING_HELPER_HPP)
#define MISC_STRING_HELPER_HPP

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

#if !defined(TRAFO_NAMESPACE)
    #define TRAFO_NAMESPACE fw
#endif

namespace TRAFO_NAMESPACE
{
    namespace internal
    {
        bool findAndReplace(std::string& input, const std::string& findString, const std::string& replaceString)
        {
            const std::string::size_type pos = input.find(findString);
            
            if (pos != std::string::npos)
            {
                input.replace(pos, findString.length(), replaceString);
                return true;
            }

            return false;
        }

        static std::vector<std::string> splitString(const std::string& input, const char delimiter)
        {
            std::vector<std::string> output;
            std::string tmp;
            std::istringstream inputStream(input);

            while (std::getline(inputStream, tmp, delimiter))
            {
                if (tmp != std::string(""))
                {
                    output.push_back(tmp);
                }
            }

            return output;
        }

        static std::string removeSpaces(const std::string& input)
        {
            std::string output(input);

            output.erase(std::remove_if(output.begin(), output.end(), ::isspace), output.end());

            return output;
        }

        static std::string concat(const std::vector<std::string>& input, const std::string delimiter = std::string(""))
        {
            std::string output;
            const std::uint32_t numElements = input.size();
            std::uint32_t elementId = 0;

            for (const std::string element : input)
            {
                output += element + (++elementId < numElements ? delimiter : std::string(""));
            }

            return output;
        }
    }
}

#endif