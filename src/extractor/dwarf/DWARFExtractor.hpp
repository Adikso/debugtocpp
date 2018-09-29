#ifndef DEBUGTOCPP_DWARFEXTRACTOR_HPP
#define DEBUGTOCPP_DWARFEXTRACTOR_HPP

#include <string.h>

#include "../Extractor.hpp"
#include <libelfin/elf/elf++.hh>
#include <libelfin/dwarf/dwarf++.hh>

namespace debugtocpp {
namespace dwarf {

class DWARFExtractor : public Extractor {
public:
    ExtractResult load(std::string filename, int image_base) override;

    Type *getType(std::string name) override;
    std::vector<Type *> getTypes(std::list<std::string> typesList) override;

    std::list<std::string> getTypesList(bool showStructs) override;
    std::vector<Field *> getAllGlobalVariables() override;

private:
    ::elf::elf * elf;
    ::dwarf::dwarf * dwarf;

    Type *getType(const ::dwarf::die &node, std::string &name);
    ::dwarf::die findTypeNode(const ::dwarf::die &node, std::string &name);
    TypePtr * getTypePtr(const ::dwarf::die &die);
    void updateMethods(std::vector<Method *> &methods, const ::dwarf::unit &unit, ::dwarf::section_offset offset);
};

}
}

#endif //DEBUGTOCPP_DWARFEXTRACTOR_HPP
