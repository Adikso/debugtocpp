#ifndef DEBUGTOCPP_CODECLASSDUMPER_HPP
#define DEBUGTOCPP_CODECLASSDUMPER_HPP

#include "ClassDumper.hpp"
#include "utils/cxxopts.h"

using namespace debugtocpp;

namespace debugtocpp {

class CodeClassDumper : public ClassDumper {
public:
    std::string dump(Type * cls, DumpConfig config) override;
    std::vector<std::string> dump(std::vector<Type *> type, DumpConfig config) override;

private:
    static std::string invalidCharacters;

    void dumpPointers(std::stringstream &out, Type * cls, DumpConfig config);
    void dumpMethodArgs(std::stringstream &out, Method * method, bool pointers, DumpConfig config);
    std::string getName(std::string fullName, Type *cls);
    std::string printType(TypePtr * type, bool compilable);
};

}

#endif //DEBUGTOCPP_CODECLASSDUMPER_HPP
