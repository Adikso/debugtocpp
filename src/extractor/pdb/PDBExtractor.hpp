#ifndef DEBUGTOCPP_PDBEXTRACTOR_HPP
#define DEBUGTOCPP_PDBEXTRACTOR_HPP

#include "../Extractor.hpp"
#include "../../common/DebugTypes.hpp"

namespace debugtocpp {
namespace pdb {

class PDBExtractor : public Extractor {
public:
    ExtractResult load(std::string filename, int image_base) override;

    Type *getType(std::string name) override;
    std::vector<Type *> getTypes(std::list<std::string> typesList) override;
    std::list<std::string> getTypesList(bool showStructs) override;

private:
    retdec::pdbparser::PDBFile pdb;

    TypePtr * getReturnType(PDBTypeDef *type, int flags = 0);
    Method *getMethod(PDBFunction * func);
    Method *getMethod(PDBTypeFieldMember *fieldMember);

    PDBTypeDef *findFullDeclaration(std::string name);
};

class PDBUniversalType {
public:
    std::string getName();
    int getFieldListTypeId();
    ePDBTypeClass getPDBType();
    bool isType();

    explicit PDBUniversalType(PDBTypeDef * typeDef);

private:
    PDBTypeClass *pdbClass = nullptr;
    PDBTypeStruct *pdbStruct = nullptr;
};

}
}

#endif //DEBUGTOCPP_PDBEXTRACTOR_HPP
