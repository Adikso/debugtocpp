#ifndef DEBUGTOCPP_UTILS_HPP
#define DEBUGTOCPP_UTILS_HPP

#include <string>
#include <list>
#include <iostream>
#include <sstream>
#include <sys/stat.h>
#include <vector>
#include <algorithm>
#include <iterator>
#include "string.h"

inline std::string clearString(std::string str) {
    for (char &c : str) {
        if ((c >= ' ' && c <= '/') || (c >= ':' && c <= '@') || (c >= '[' && c <= '`')) {
            c = '_';
        }
    }

    return str;
}

inline std::list<std::string> split(const std::string &input, char delimiter) {
    std::istringstream ss(input);
    std::list<std::string> tokens;
    std::string token;

    while(std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }

    return tokens;
}

inline std::string join(std::vector<std::string> &source, std::string delim) {
    std::string out;

    for (auto &type : source) {
        out.append(type).append(delim);
    }

    return out;
}

inline int isDirectory(const std::string &path) {
    struct stat statbuf{};
    if (stat(path.c_str(), &statbuf) != 0)
        return 0;
    return S_ISDIR(statbuf.st_mode);
}

// This is hack
inline std::string demangleName(const std::string &mangled) {
    std::string prefix = "_ZT";
    std::string prefix2 = "_ZN";
    if (mangled.rfind(prefix, 0) == 0 || mangled.rfind(prefix2, 0) == 0) {
        std::string rest = mangled.substr(prefix.size()); // Get everything without prefix

        // Skip all letters until size found
        while (!std::isdigit(rest[0]) && rest.size() > 1) {
            rest = rest.substr(1);
        }

        std::vector<std::string> parts;

        while (true) {
            std::stringstream ss;
            ss << rest;
            int length;
            ss >> length;

            if (length == 0 || rest.empty()) {
                if (rest[0] == 'D') { // Destructor
                    parts.back().insert(0, "~");
                }
                break;
            }

            if (length + std::to_string(length).length() > rest.length())
                break;

            std::string part = rest.substr(std::to_string(length).length(), length);
            parts.push_back(part);

            rest = rest.substr(std::to_string(length).length() + length);
        }

        // Leave class name only
        if (parts.size() > 1 && mangled.rfind(prefix2, 0) == 0) {
            parts.pop_back();
        }

        std::stringstream out;

        for (int i = 0; i < parts.size(); i++) {
            out << parts[i];

            if (i != parts.size() - 1) {
                out << "::";
            }
        }

        return out.str();
    }

    return std::string();
}

#endif //DEBUGTOCPP_UTILS_HPP
