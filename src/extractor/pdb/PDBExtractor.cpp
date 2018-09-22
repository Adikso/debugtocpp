#include <cstring>
#include <list>
#include "PDBExtractor.hpp"
#include "../../utils/utils.hpp"

namespace debugtocpp {
namespace pdb {

ExtractResult PDBExtractor::load(std::string filename, int image_base) {
    pdb = PDBFile();

    switch (pdb.load_pdb_file(filename.c_str())) {
        case PDB_STATE_ERR_FILE_OPEN:
            return ExtractResult::ERR_FILE_OPEN;
        case PDB_STATE_INVALID_FILE:
            return ExtractResult::INVALID_FILE;
        case PDB_STATE_UNSUPPORTED_VERSION:
            return ExtractResult::UNSUPPORTED_VERSION;
        case PDB_STATE_ALREADY_LOADED:break;
        case PDB_STATE_OK:
            pdb.initialize(image_base);
            return ExtractResult::OK;
    }

    return ExtractResult::UNKNOWN_ERROR;
}

Type *PDBExtractor::getType(std::string name) {
    allDependentClasses.clear();

    auto * pdbType = new PDBUniversalType(findFullDeclaration(name));

    // Type = class or struct
    if (!pdbType->isType()) {
        return nullptr;
    }

    Type * type = new Type(pdbType->getName());
    PDBTypeFieldList * fieldsList = reinterpret_cast<PDBTypeFieldList *>(pdb.get_types_container()->types[pdbType->getFieldListTypeId()]);

    // Load all fields (methods of class are also fields)
    if (fieldsList != nullptr) {
        for (auto &pdbField : fieldsList->fields) {

            // Load base classes/structs
            if (pdbField.field_type == PDBFIELD_BASE) {
                auto pdbBaseType = new PDBUniversalType(pdbField.Member.type_def);
                type->baseTypes.push_back(new Type(pdbBaseType->getName()));
                delete(pdbBaseType);
                continue;
            }

            // Load methods TODO METHOD
            if (pdbField.field_type == PDBFIELD_ONEMETHOD) {
                type->allMethods.push_back(getMethod(&pdbField.Member));
                continue;
            }

            // Load fields
            Field * field = new Field(pdbField.Member.name, getReturnTypeStr(pdbField.Member.type_def), pdbField.Member.offset);
            field->accessibility = Accessibility::PUBLIC;

            if (pdbField.field_type == PDBFIELD_STMEMBER) {
                field->isStatic = true;
            }

            type->fields.push_back(field);

        }
    }

    // Load all methods
    for (auto &func : *pdb.get_functions()) {
        // ??
        if (dynamic_cast<PDBTypeClass *>(func.second->type_def->func_clstype_def)) {
            auto parentClass = (PDBTypeClass *) func.second->type_def->func_clstype_def;

            if (strcmp(parentClass->class_name, type->name.c_str()) == 0) {
                Method *method = getMethod(func.second);

                // Apply more details to already found methods
                for (int i = 0; i < type->allMethods.size(); i++) {
                    if (type->allMethods[i]->name == method->name.substr(type->name.size() + 2)) {
                        type->allMethods[i]->args = method->args;
                        type->allMethods[i]->address = method->address;
                        type->allMethods[i]->isStatic = method->isStatic;
                        // STATIC nie zawsze się pojawia i ogólnie problemy
                    }
                }

                type->fullyDefinedMethods.push_back(method);
            }
        }
    }

    if (!type->baseTypes.empty()) {
        allDependentClasses.push_back(type->baseTypes[0]->name);
    }

    allDependentClasses.sort();
    allDependentClasses.unique();
    type->dependentTypes = allDependentClasses;

    delete(pdbType);
    return type;
}

Method *PDBExtractor::getMethod(std::string name) {
    for (auto &func : *this->pdb.get_functions()) {
        if (strcmp(func.second->name, name.c_str()) == 0) {
            return getMethod(func.second);
        }
    }

    return nullptr;
}

Method *PDBExtractor::getMethod(PDBFunction *func) {
    auto * method = new Method();

    method->name = func->name;
    if (method->name[0] == '~') { // Fixes destructor returning void
        method->returnType = new TypePtr("", true);
    } else {
        method->returnType = getReturnTypeStr(func->type_def->func_rettype_def);
    }

    method->address = func->address;
    method->callType = func->type_def->func_calltype;
    method->isStatic = func->type_def->func_thistype_index == 0;
    method->isVariadic = func->type_def->func_is_variadic;

    std::vector<Argument *> args;
    for (auto &fArg : func->arguments) {
        PDBTypeDef *argumentType = pdb.get_types_container()->get_type_by_index(fArg.type_index);

        Argument *arg = new Argument(fArg.name, getReturnTypeStr(argumentType));
        args.push_back(arg);
    }
    method->args = args;

    return method;
}

Method *PDBExtractor::getMethod(PDBTypeFieldMember *fieldMember) {
    auto * pdbTypeFunction = (PDBTypeFunction *) fieldMember->type_def;
    auto * method = new Method();

    method->name = fieldMember->name;
    if (method->name[0] == '~') { // Fixes destructor returning void
        method->returnType = new TypePtr("", true);
    } else {
        method->returnType = getReturnTypeStr(pdbTypeFunction->func_rettype_def);
    }

    method->callType = pdbTypeFunction->func_calltype;
    method->isStatic = pdbTypeFunction->func_thistype_index == 0;
    method->isVariadic = pdbTypeFunction->func_is_variadic;
    method->isVirtual = fieldMember->offset == -1;
    method->accessibility = Accessibility::PUBLIC;

    for (int i = 0; i < pdbTypeFunction->func_args_count; i++) {
        Argument * arg = new Argument("", getReturnTypeStr(pdbTypeFunction->func_args[i].type_def));
        method->args.push_back(arg);
    }

    return method;
}

TypePtr * PDBExtractor::getReturnTypeStr(PDBTypeDef *type) {
    if (dynamic_cast<PDBTypeBase *>(type)) {
        return new TypePtr(((PDBTypeBase *) type)->description, false);
    } else if (dynamic_cast<PDBTypePointer *>(type)) {
        return getReturnTypeStr(((PDBTypePointer *) type)->ptr_utype_def); // Follows pointer to the type
    }

    auto pdbType = new PDBUniversalType(type);
    if (!pdbType->isType()) {
        delete(pdbType);
        return new TypePtr("int", false);
    }

    std::string name = pdbType->getName();
    allDependentClasses.push_back(name);

    delete(pdbType);
    return new TypePtr(name, true);
}

PDBTypeDef *PDBExtractor::findFullDeclaration(std::string name) {
    PDBTypeDef * last = nullptr;

    for (auto &type : pdb.get_types_container()->types_fully_defined) {
        auto pdbType = new PDBUniversalType(type.second);
        if (pdbType->isType()) {
            if (pdbType->getName() != name) {
                delete(pdbType);
                continue;
            }

            last = type.second;
        }
        delete(pdbType);
    }

    return last;
}

std::list<std::string> PDBExtractor::getTypesList(bool showStructs) {
    std::list<std::string> names;

    for (auto &type : pdb.get_types_container()->types_fully_defined) {
        auto pdbType = new PDBUniversalType(type.second);
        if (!pdbType->isType() || (pdbType->getPDBType() == PDBTYPE_STRUCT && !showStructs)) {
            delete(pdbType);
            continue;
        }

        names.emplace_back(pdbType->getName());
        delete(pdbType);
    }

    names.sort();
    names.unique();

    return names;
}

// PDBUniversalType methods

std::string PDBUniversalType::getName() {
    if (pdbClass != nullptr) {
        return pdbClass->class_name;
    } else if (pdbStruct != nullptr) {
        return pdbStruct->struct_name;
    }

    return std::__cxx11::string();
}

PDBUniversalType::PDBUniversalType(PDBTypeDef * typeDef) {
    if (dynamic_cast<PDBTypeClass *>(typeDef)) {
        pdbClass = (PDBTypeClass *) typeDef;
    }
    else if (dynamic_cast<PDBTypeStruct *>(typeDef)) {
        pdbStruct = (PDBTypeStruct *) typeDef;
    }
}

int PDBUniversalType::getFieldListTypeId() {
    if (pdbClass != nullptr) {
        return pdbClass->field;
    } else if (pdbStruct != nullptr) {
        return pdbStruct->field;
    }

    return -1;
}

ePDBTypeClass PDBUniversalType::getPDBType() {
    if (pdbClass != nullptr) {
        return pdbClass->type_class;
    } else if (pdbStruct != nullptr) {
        return pdbStruct->type_class;
    }

    return PDBTYPE_CONST;
}

bool PDBUniversalType::isType() {
    return getPDBType() == PDBTYPE_CLASS || getPDBType() == PDBTYPE_STRUCT;
}
}

}
