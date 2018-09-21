#ifndef DEBUGTOCPP_UTILS_HPP
#define DEBUGTOCPP_UTILS_HPP

#include <string>
#include <list>
#include <iostream>
#include <sstream>
#include <sys/stat.h>
#include "string.h"

inline std::string * clearString(std::string * str) {
    for (char &c : *str) {
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

inline int isDirectory(const std::string &path) {
    struct stat statbuf{};
    if (stat(path.c_str(), &statbuf) != 0)
        return 0;
    return S_ISDIR(statbuf.st_mode);
}

// This is hack TODO
inline std::string demangleTypedef(const std::string &mangled) {
    std::string prefix = "_ZTI";
    if (mangled.rfind(prefix, 0) == 0) {
        std::string rest = mangled.substr(prefix.size()); // Get everything without prefix

        bool nspace = rest[0] == 'N';
        if (nspace) {
            rest = rest.substr(1);
        }

        std::stringstream out;

        while (true) {
            std::stringstream ss;
            ss << rest;
            int length;
            ss >> length;

            if (length == 0)
                break;

            out << rest.substr(std::to_string(length).length(), length);
            rest = rest.substr(std::to_string(length).length() + length);

            if (std::isdigit(rest[0]) && nspace) {
                out << "::";
            }
        }

        return out.str();
    }

    return std::string();
}

#endif //DEBUGTOCPP_UTILS_HPP
