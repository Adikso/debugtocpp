#ifndef DEBUGTOCPP_EXTRACTOR_HPP
#define DEBUGTOCPP_EXTRACTOR_HPP

#include <string>
#include <list>
#include "../common/DebugTypes.hpp"

using namespace debugtocpp::types;

namespace debugtocpp {

enum ExtractResult {
    ERR_FILE_OPEN, INVALID_FILE, UNSUPPORTED_VERSION, OK, UNKNOWN_ERROR
};

class DebugExtractException : public std::exception {
public:
    std::string msg;

    virtual std::string reason() const noexcept {
        return msg;
    }

    explicit DebugExtractException(const std::string &msg) : msg(msg) {}
};

class Extractor {
public:
    virtual ExtractResult load(std::string filename, int image_base) = 0;

    virtual Type *getType(std::string name) = 0;

    virtual std::list<std::string> getTypesList(bool showStructs) = 0;

    virtual Method *getMethod(std::string name) = 0;

protected:
    std::list<std::string> allDependentClasses;
};

}

#endif //DEBUGTOCPP_EXTRACTOR_HPP
