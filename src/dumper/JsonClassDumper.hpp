#ifndef DEBUGTOCPP_JSONCLASSDUMPER_HPP
#define DEBUGTOCPP_JSONCLASSDUMPER_HPP

#include "ClassDumper.hpp"
#include "../utils/cxxopts.h"
#include "../utils/json.hpp"

using namespace debugtocpp;
using json = nlohmann::json;

namespace debugtocpp {

class JsonClassDumper : public ClassDumper {
public:
    std::string dump(Type *cls, DumpConfig config) override;
    std::vector<std::string> dump(std::vector<Type *> type, DumpConfig config) override;

private:
    json dumpAsJsonObj(Type *cls);
};

}

#endif //DEBUGTOCPP_JSONCLASSDUMPER_HPP
