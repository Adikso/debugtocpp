#include <fcntl.h>
#include "extractor/dwarf/DWARFExtractor.hpp"
#include <iostream>

namespace debugtocpp {
namespace dwarf {

ExtractResult debugtocpp::dwarf::DWARFExtractor::load(std::string filename, int image_base) {
    int fd = open(filename.c_str(), O_RDONLY);

    if (fd < 0) {
        return ExtractResult::ERR_FILE_OPEN;
    }

    try {
        elf = new ::elf::elf(::elf::create_mmap_loader(fd));
        dwarf = new ::dwarf::dwarf(::dwarf::elf::create_loader(*elf));
    } catch (::elf::format_error &e) {
        if (strcmp(e.what(), "bad ELF magic number") == 0) {
            return ExtractResult::INVALID_FILE;
        } else {
            return ExtractResult::UNSUPPORTED_VERSION;
        }
    } catch (::dwarf::format_error &e) {
        if (strcmp(e.what(), "required .debug_info section missing") == 0) {
            return ExtractResult::MISSING_DEBUG;
        }
    }

    return ExtractResult::OK;
}

Type *debugtocpp::dwarf::DWARFExtractor::getType(std::string name) {
    for (auto &cu : dwarf->compilation_units()) {
        ::dwarf::die result = findTypeNode(cu.root(), name);

        if (result.valid()) {
            return getType(result, name);
        }
    }

    return nullptr;
}

Type *DWARFExtractor::getType(const ::dwarf::die &node, std::string &name) {
    Type * type = new Type(name);
    std::vector<Method *> methods;

    for (const ::dwarf::die &child : node) {

        if (child.tag == ::dwarf::DW_TAG::inheritance) {
            for (auto &attr : child.attributes()) {
                if (attr.first == ::dwarf::DW_AT::type) {
                    Type * baseType = new Type();

                    for (auto &subAttr : attr.second.as_reference().attributes()) {
                        if (subAttr.first == ::dwarf::DW_AT::name) {
                            baseType->name = subAttr.second.as_string();
                        }
                    }

                    type->baseTypes.push_back(baseType);
                }
            }
        }

        if (child.tag == ::dwarf::DW_TAG::member) {
            auto * field = new Field();

            for (auto &attr : child.attributes()) {
                if (attr.first == ::dwarf::DW_AT::name) {
                    field->name = attr.second.as_string();
                }

                if (attr.first == ::dwarf::DW_AT::type) {
                    field->typePtr = getTypePtr(attr.second.as_reference());
                }

                if (attr.first == ::dwarf::DW_AT::external) {
                    field->isStatic = attr.second.as_flag();
                }

                if (attr.first == ::dwarf::DW_AT::accessibility) {
                    field->accessibility = static_cast<Accessibility>(attr.second.as_uconstant());
                }
            }
            type->fields.push_back(field);
        }

        if (child.tag == ::dwarf::DW_TAG::subprogram) {
            auto * method = new Method();
            method->returnType = new TypePtr("void", false);

            for (auto &attr : child.attributes()) {
                if (attr.first == ::dwarf::DW_AT::name) {
                    method->name = attr.second.as_string();
                }

                if (attr.first == ::dwarf::DW_AT::type) {
                    delete(method->returnType);
                    method->returnType = getTypePtr(attr.second.as_reference());
                }

                if (attr.first == ::dwarf::DW_AT::linkage_name) {
                    method->mangledName = attr.second.as_string();
                }

                if (attr.first == ::dwarf::DW_AT::accessibility) {
                    method->accessibility = static_cast<Accessibility>(attr.second.as_uconstant());
                }

                if (attr.first == ::dwarf::DW_AT::artificial) {
                    method->isCompilerGenerated = attr.second.as_flag();
                }
            }

            if (method->name == type->name) {
                method->returnType->type = "";
            }

            methods.push_back(method);
        }
    }

    updateMethods(methods, node.get_unit(), node.get_section_offset());
    type->allMethods = methods;
    type->fullyDefinedMethods = methods;

    return type;
}

TypePtr * DWARFExtractor::getTypePtr(const ::dwarf::die &die) {
    TypePtr * typePtr = new TypePtr("unknown", true);

    if (die.tag == ::dwarf::DW_TAG::pointer_type || die.tag == ::dwarf::DW_TAG::const_type ||
        die.tag == ::dwarf::DW_TAG::reference_type || die.tag == ::dwarf::DW_TAG::array_type) {

        for (auto &attr : die.attributes()) {
            if (attr.first == ::dwarf::DW_AT::type) {
                TypePtr * type = getTypePtr(attr.second.as_reference());

                type->isPointer = die.tag == ::dwarf::DW_TAG::pointer_type;
                type->isConstant = die.tag == ::dwarf::DW_TAG::const_type;
                type->isReference = die.tag == ::dwarf::DW_TAG::reference_type;
                type->isArray = die.tag == ::dwarf::DW_TAG::array_type;

                if (type->isArray) {
                    for (const ::dwarf::die &arrayChild : die) {
                        if (arrayChild.tag == ::dwarf::DW_TAG::subrange_type) {
                            for (auto &arrayAttr : arrayChild.attributes()) {
                                if (arrayAttr.first == ::dwarf::DW_AT::upper_bound) {
                                    type->arraySize = arrayAttr.second.as_sconstant() + 1;
                                }
                            }
                        }
                    }
                }

                return type;
            }
        }
    }

    if (die.tag == ::dwarf::DW_TAG::base_type || die.tag == ::dwarf::DW_TAG::class_type) {
        typePtr->isPointer = die.tag == ::dwarf::DW_TAG::class_type;
        typePtr->isBaseType = die.tag == ::dwarf::DW_TAG::base_type;
        for (auto &attr : die.attributes()) {
            if (attr.first == ::dwarf::DW_AT::name) {
                typePtr->type = attr.second.as_string();
            }
        }
    }

    return typePtr;
}

::dwarf::die debugtocpp::dwarf::DWARFExtractor::findTypeNode(const ::dwarf::die &node, std::string &name) {
    if (node.tag == ::dwarf::DW_TAG::class_type) {
        for (auto &attr : node.attributes()) {
            if (attr.first == ::dwarf::DW_AT::name && attr.second.as_string() == name) {
                return node;
            }
        }
    }

    for (auto &child : node) {
        auto found = findTypeNode(child, name);
        if (found.valid()) {
            return found;
        }
    }

    return ::dwarf::die();
}

void DWARFExtractor::updateMethods(std::vector<Method *> &methods, const ::dwarf::unit &unit, ::dwarf::section_offset offset) {
    for (auto &cu : dwarf->compilation_units()) {
        for (const ::dwarf::die &child : cu.root()) {
            if (child.tag == ::dwarf::DW_TAG::subprogram) {
                if (child.attributes()[0].first != ::dwarf::DW_AT::specification) {
                    continue;
                }

                Method *method = nullptr;

                for (auto &refAttrs : child.attributes()[0].second.as_reference().attributes()) {
                    if (refAttrs.first == ::dwarf::DW_AT::linkage_name) {
                        for (auto &m : methods) {
                            if (m->mangledName == refAttrs.second.as_string()) {
                                method = m;
                            }
                        }
                    }
                }

                if (method == nullptr) {
                    continue;
                }

                for (const ::dwarf::die &subChild : child) {
                    if (subChild.tag == ::dwarf::DW_TAG::formal_parameter) {
                        auto *arg = new Argument();

                        for (auto &attr : subChild.attributes()) {
                            if (attr.first == ::dwarf::DW_AT::name) {
                                arg->name = attr.second.as_string();
                            }

                            if (attr.first == ::dwarf::DW_AT::type) {
                                arg->typePtr = getTypePtr(attr.second.as_reference());
                            }
                        }

                        method->args.push_back(arg);
                    }
                }
            }
        }
    }
}

std::list<std::string> debugtocpp::dwarf::DWARFExtractor::getTypesList(bool showStructs) {
    return std::list<std::string>();
}

std::vector<Type *> DWARFExtractor::getTypes(std::list<std::string> typesList) {
    std::vector<Type *> types;
    // No optimization here yet
    for (std::string &name : typesList) {
        types.push_back(getType(name));
    }

    return types;
}

std::vector<Field *> DWARFExtractor::getAllGlobalVariables() {
    return std::vector<Field *>();
}


}
}