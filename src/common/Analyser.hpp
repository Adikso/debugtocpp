#ifndef DEBUGTOCPP_ANALYSER_HPP
#define DEBUGTOCPP_ANALYSER_HPP

#include "DebugTypes.hpp"
#include "../dumper/ClassDumper.hpp"

using namespace debugtocpp;

namespace debugtocpp {

class Analyser {
public:
    Analyser(const DumpConfig &config) : config(config) {}

    void process(std::vector<Type *> &types);
    bool process(Type * type);
    bool isCompilerGeneratedType(std::string &name);
    bool isCompilerGenerated(Type * type);
    bool isCompilerGenerated(Method * method);
private:
    DumpConfig config;
};

}

#endif //DEBUGTOCPP_ANALYSER_HPP
