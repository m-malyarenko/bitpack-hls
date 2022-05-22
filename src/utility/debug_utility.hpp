#ifndef __UTILITY_DEBUG_UTILITY_HPP__
#define __UTILITY_DEBUG_UTILITY_HPP__

#include <string>

namespace llvm {
    namespace bphls {

bool replaceAll(std::string &haystack, const std::string &needle, const std::string &replace);

void limitString(std::string &s, unsigned limit);

    }
}

#endif /* __UTILITY_DEBUG_UTILITY_HPP__ */
