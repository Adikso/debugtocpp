#include <cstring>
#include <list>
#include "extractor/pdb/PDBExtractor.hpp"
#include "utils/utils.hpp"

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

            if (pdbField.field_type == PDBFIELD_NESTTYPE) {
                Type * nestedType = getType(type->name + "::" + pdbField.Member.name);
                if (nestedType != nullptr) {
                    nestedType->name = pdbField.Member.name;
//                    type->nestedTypes.push_back(nestedType);
                } else {
                    nestedType = new Type(pdbField.Member.name);
//                    type->nestedTypes.push_back(nestedType);
                }
                type->nestedTypes.push_back(nestedType);
                continue;
            }

            // Load fields
            Field * field = new Field(pdbField.Member.name, getReturnType(pdbField.Member.type_def), pdbField.Member.offset);
            field->accessibility = Accessibility::PUBLIC;

            if (pdbField.field_type == PDBFIELD_STMEMBER) {
                field->isStatic = true;
            }

            type->fields.push_back(field);

        }

        // Search global variables for addresses
        for (auto &globalVar : *pdb.get_global_variables()) {
            for (auto &field : type->fields) {
                if (globalVar.second.name == name + "::" + field->name) {
                    field->address = (unsigned long) globalVar.second.address;
                }
            }
        }
    }

    // Load all methods
    for (auto &func : *pdb.get_functions()) {
        // ??
        if (dynamic_cast<PDBTypeClass *>(func.second->type_def->func_clstype_def)) {
            auto parentClass = (PDBTypeClass *) func.second->type_def->func_clstype_def;

            if (strcmp(parentClass->class_name, type->name.c_str()) == 0) {
                Method *method = getMethod(func.second);

                bool found = false;
                // Apply more details to already found methods
                for (int i = 0; i < type->allMethods.size(); i++) {
                    if (type->allMethods[i]->name == method->name.substr(type->name.size() + 2)) {
                        int offset = (!method->args.empty() && method->args[0]->name == "this" ? 1 : 0);
                        if (method->args.size() - offset != type->allMethods[i]->args.size()) {
                            continue;
                        }

                        for (int j = offset; j < method->args.size(); j++) {
                            if (*method->args[j]->typePtr != *type->allMethods[i]->args[j - offset]->typePtr) {
                                goto continue_loop;
                            }
                        }

                        type->allMethods[i]->args = method->args;
                        type->allMethods[i]->address = method->address;
                        type->allMethods[i]->isStatic = method->isStatic;

                        found = true;
                        break;
                    }
                    continue_loop: continue;
                }

                if (!found) {
                    method->name = method->name.substr(type->name.size() + 2);
                    type->allMethods.push_back(method);
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

Method *PDBExtractor::getMethod(PDBFunction *func) {
    auto * method = new Method();

    method->name = func->name;
    if (method->name[0] == '~') { // Fixes destructor returning void
        method->returnType = new TypePtr("", false);
    } else {
        method->returnType = getReturnType(func->type_def->func_rettype_def);
    }

    method->address = func->address;
    method->callType = func->type_def->func_calltype;
    method->isStatic = func->type_def->func_thistype_index == 0;
    method->isVariadic = func->type_def->func_is_variadic;
    method->accessibility = Accessibility::PUBLIC;

    std::vector<Argument *> args;
    for (auto &fArg : func->arguments) {
        PDBTypeDef *argumentType = pdb.get_types_container()->get_type_by_index(fArg.type_index);

        Argument *arg = new Argument(fArg.name, getReturnType(argumentType));
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
        method->returnType = new TypePtr("", false);
    } else {
        method->returnType = getReturnType(pdbTypeFunction->func_rettype_def);
    }

    method->callType = pdbTypeFunction->func_calltype;
    method->isStatic = pdbTypeFunction->func_thistype_index == 0;
    method->isVariadic = pdbTypeFunction->func_is_variadic;
    method->isVirtual = fieldMember->offset != -1;
    method->vftableOffset = fieldMember->offset;
    method->accessibility = Accessibility::PUBLIC;

    for (int i = 0; i < pdbTypeFunction->func_args_count; i++) {
        Argument * arg = new Argument("", getReturnType(pdbTypeFunction->func_args[i].type_def));
        method->args.push_back(arg);
    }

    return method;
}

TypePtr * PDBExtractor::getReturnType(PDBTypeDef *type, int flags) {
    if (dynamic_cast<PDBTypeBase *>(type)) {
        auto * typeBase = (PDBTypeBase *) type;
        TypePtr *typePtr = new TypePtr(typeBase->description, false);
        typePtr->isBaseType = true;

        if (flags & 1) typePtr->isConstant = true;
        if (flags & 2) typePtr->isPointer = true;

        return typePtr;
    } else if (dynamic_cast<PDBTypeConst *>(type)) {
        return getReturnType(((PDBTypeConst *) type)->const_utype_def, flags | 1);
    } else if (dynamic_cast<PDBTypePointer *>(type)) {
        return getReturnType(((PDBTypePointer *) type)->ptr_utype_def, flags | 2); // Follows pointer to the type
    }

    auto pdbType = new PDBUniversalType(type);
    if (!pdbType->isType()) {
        delete(pdbType);
        return new TypePtr("int", false);
    }

    std::string name = pdbType->getName();
    allDependentClasses.push_back(name);

    delete(pdbType);
    auto * typePtr = new TypePtr(name, true);

    if (flags & 1) typePtr->isConstant = true;
    if (flags & 2) typePtr->isPointer = true;

    return typePtr;
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

std::vector<Type *> PDBExtractor::getTypes(std::list<std::string> typesList) {
    std::vector<Type *> types;
    // No optimization here yet
    for (std::string &name : typesList) {
        types.push_back(getType(name));
    }

    return types;
}

std::vector<Field *> PDBExtractor::getAllGlobalVariables() {
    std::vector<Field *> fields;

    for (auto &var : *pdb.get_global_variables()) {
        auto * field = new Field();
        field->name = var.second.name;
        field->address = var.second.address;
        field->accessibility = Accessibility::PUBLIC;
        field->typePtr = getReturnType(var.second.type_def);

        fields.push_back(field);
    }

    return fields;
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
