#include <string>

#include "debug_utility.hpp"

bool llvm::bphls::replaceAll(std::string &haystack, const std::string &needle, const std::string &replace) {
    size_t i, from = 0;
    bool found = false;

    while ((i = haystack.find(needle, from)) != std::string::npos) {
        haystack.replace(i, needle.length(), replace);
        from = i + replace.length();
        found = true;
    }

    return found;
}

void llvm::bphls::limitString(std::string &s, unsigned limit) {
    if (s.length() > limit) {
        s.erase(limit);
        s = s + "...";
    }
}