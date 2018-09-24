#ifndef DEBUGTOCPP_ELFEXTRACTOR_HPP
#define DEBUGTOCPP_ELFEXTRACTOR_HPP

#include "../Extractor.hpp"
#include "retdec/demangler/demangler.h"
#include <libelfin/elf/elf++.hh>

namespace debugtocpp {
namespace elf {

class ELFExtractor : public Extractor {
public:
    ExtractResult load(std::string filename, int image_base) override;

    Type *getType(std::string name) override;
    std::vector<Type *> getTypes(std::list<std::string> typesList) override;
    std::list<std::string> getTypesList(bool showStructs) override;

    Method *getMethod(std::string name) override;

private:
    Method *getMethod(::elf::sym * element);
    TypePtr * getTypePtr(retdec::demangler::cName &cname, retdec::demangler::cName::type_t &ttype);
    bool isDuplicated(Type * type, Method * method);

    ::elf::elf * elf;
    ::elf::symtab symtab;
    std::unique_ptr<retdec::demangler::CDemangler> demangler;
};

}
}

#endif //DEBUGTOCPP_ELFEXTRACTOR_HPP
