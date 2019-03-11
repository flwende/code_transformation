// Copyright (c) 2017-2018 Florian Wende (flwende@gmail.com)
//
// Distributed under the BSD 2-clause Software License
// (See accompanying file LICENSE)

#if !defined(MISC_STRING_HELPER_HPP)
#define MISC_STRING_HELPER_HPP

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <regex>
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
        bool find(const std::string& input, const std::string& findString, const bool matchWholeWord = false)
        {   
            // match all occurences of 'findString' with
            //   a) leading character not in {'a-z' 'A-Z' '0-9'} or leading linestart (^)
            //   b) succeeding characted not in {'a-z' 'A-Z' '0-9'} or succeeding lineend ($)
            //
            // Note 1: we use three captures ($1)($2)($3) with $0 being the match itself and $2 being 'findString'
            // Note 2: the last capture is not part of the overall regular expression (?=..)
            const std::string regPrefix = std::string(matchWholeWord ? "(^|[^a-zA-Z0-9_])" : "()");
            const std::string regSuffix = std::string(matchWholeWord ? "(?=$|[^a-zA-Z0-9_])" : "()");
            const std::regex regExpr(regPrefix + findString + regSuffix);
            std::smatch regMatch;
            const std::string suffixString(input);

            return std::regex_search(suffixString, regMatch, regExpr);
        }        

        bool findAndReplace(std::string& input, const std::string& findString, const std::string& replaceString, const bool findAll = false, const bool matchWholeWord = false)
        {   
            // match all occurences of 'findString' with
            //   a) leading character not in {'a-z' 'A-Z' '0-9'} or leading linestart (^)
            //   b) succeeding characted not in {'a-z' 'A-Z' '0-9'} or succeeding lineend ($)
            //
            // Note 1: we use three captures ($1)($2)($3) with $0 being the match itself and $2 being 'findString'
            // Note 2: the last capture is not part of the overall regular expression (?=..)
            const std::string regPrefix = std::string(matchWholeWord ? "(^|[^a-zA-Z0-9_])" : "()");
            const std::string regSuffix = std::string(matchWholeWord ? "(?=$|[^a-zA-Z0-9_])" : "()");
            const std::regex regExpr(regPrefix + findString + regSuffix);
            std::smatch regMatch;
            std::string suffixString(input);
            bool match = false;

            while(std::regex_search(suffixString, regMatch, regExpr))
            {
                // there was at least one match
                match = true;

                // match position: start of 'findString': original input length minus the length of the remaining suffix minus the length of 'findString'
                const std::size_t pos = input.length() - (regMatch.suffix().length() + findString.length());
                input.replace(pos, findString.length(), replaceString);

                if (!findAll) break;

                // continue with the remaining suffix string
                suffixString = regMatch.suffix().str();
            }

            return match;
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

            output.erase(std::remove_if(output.begin(), output.end(), [] (const char c) { return std::isspace(c); }), output.end());

            return output;
        }

        static std::string removeNewline(const std::string& input)
        {
            std::string output(input);

            output.erase(std::remove_if(output.begin(), output.end(), [] (const char c) { return (c == '\n'); }), output.end());

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