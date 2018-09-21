#ifndef DEBUGTOCPP_CLASSDUMPER_H
#define DEBUGTOCPP_CLASSDUMPER_H

#include <string>
#include "../common/DebugTypes.hpp"
#include "../utils/cxxopts.h"

using namespace debugtocpp::types;

namespace debugtocpp {

struct DumpConfig {
    int indent = 4;
    bool json = false;
    bool noCompilerGenerated = false;
    bool showAsPointers = false;
    bool addIncludesOrDeclarations = false;
    bool useOnlyForwardDeclarations = false;
    bool addGuards = false;
    bool compilable = false;
    bool toDirectory = false;
};

class ClassDumper {
public:
    virtual std::string dump(Type * type, DumpConfig config) = 0;
    virtual std::vector<std::string> dump(std::vector<Type *> type, DumpConfig config) = 0;
};

}

#endif //DEBUGTOCPP_CLASSDUMPER_H
