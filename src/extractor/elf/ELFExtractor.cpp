#include <fcntl.h>
#include <iostream>
#include "extractor/elf/ELFExtractor.hpp"
#include "utils/utils.hpp"

using namespace retdec::demangler;

namespace debugtocpp {
namespace elf {

ExtractResult debugtocpp::elf::ELFExtractor::load(std::string filename, int image_base) {
    int fd = open(filename.c_str(), O_RDONLY);

    if (fd < 0) {
        return ExtractResult::ERR_FILE_OPEN;
    }

    try {
        elf = new ::elf::elf(::elf::create_mmap_loader(fd));
    } catch (::elf::format_error& e) {
        if (strcmp(e.what(), "bad ELF magic number") == 0) {
            return ExtractResult::INVALID_FILE;
        } else {
            return ExtractResult::UNSUPPORTED_VERSION;
        }
    }
    demangler = retdec::demangler::CDemangler::createGcc(); // TODO use other

    // Find symtab
    for (auto &section : elf->sections()) {
        if (section.get_hdr().type != ::elf::sht::symtab)
            continue;

        symtab = section.as_symtab();
    }

    // Try to find dynsym if symtab not found
    if (!symtab.valid()) {
        for (auto &section : elf->sections()) {
            if (section.get_hdr().type != ::elf::sht::dynsym)
                continue;

            symtab = section.as_symtab();
        }
    }

    if (!symtab.valid()) {
        return ExtractResult::MISSING_DEBUG;
    }

    return ExtractResult::OK;
}

Type *ELFExtractor::getType(std::string name) {
    std::list<std::string> typesList;
    typesList.push_back(name);

    std::vector<Type *> result = getTypes(typesList);
    return !result.empty() ? result[0] : nullptr;
}

std::vector<Type *> ELFExtractor::getTypes(std::list<std::string> typesList) {
    std::map<std::string, Type *> types;

    for (auto sym : symtab) {
        auto &data = sym.get_data();
        auto cname = demangler->demangleToClass(sym.get_name());
        auto mangled = sym.get_name();

        // Ignore symbols with different name than specified
        if (cname->name.empty()) {
            continue;
        }

        std::string symDemangledName = cname->printname(cname->name);
        std::string name; // garbage solution
        for (std::string &typeName : typesList) {
            if (symDemangledName.substr(0, symDemangledName.rfind("::")) == typeName) {
                name = typeName;
                break;
            }
        }

        if (name.empty()) continue;

        Type * type = types.count(name) ? types[name] : types[name] = new Type(name);

        switch (data.type()) {
            case ::elf::stt::func: {
                Method * method = getMethod(&sym);
                if (isDuplicated(type, method)) {
                    continue;
                }

                for (auto &arg : method->args) {
                    if (arg->typePtr->isPointer) {
                        type->dependentTypes.push_back(arg->typePtr->type);
                    }
                }

                if (method->name.rfind(type->name + "::") == 0) {
                    method->name = method->name.substr(type->name.size() + 2);
                }

                type->allMethods.push_back(method);
                type->fullyDefinedMethods.push_back(method);
                break;
            }
            case ::elf::stt::object: {
                std::string fieldName = cname->printname(cname->name);

                if (fieldName.rfind(type->name + "::") == 0) {
                    fieldName = fieldName.substr(type->name.size() + 2);
                }

                TypePtr * fieldType = new TypePtr(getTypeFromSize(data.size), false);

                auto * field = new Field(fieldName, fieldType, 0);
                field->isStatic = true;
                field->address = data.value;
                field->accessibility = Accessibility::PUBLIC;

                type->fields.push_back(field);
                break;
            }
        }

    }

    std::vector<Type *> typesVector;
    typesVector.reserve(types.size());
    for (auto elem : types) {
        elem.second->dependentTypes.sort();
        elem.second->dependentTypes.unique();
        typesVector.push_back(elem.second);
    }

    return typesVector;
}

std::list<std::string> ELFExtractor::getTypesList(bool showStructs) {
    std::list<std::string> names;

    for (auto sym : symtab) {
        auto &data = sym.get_data();

        std::string typedefName = demangleName(sym.get_name());
        if (typedefName.empty()) {
            continue;
        }

        names.emplace_back(typedefName);
    }

    names.sort();
    names.unique();

    return names;
}

Method *ELFExtractor::getMethod(::elf::sym *sym) {
    auto &data = sym->get_data();
    auto cname = demangler->demangleToClass(sym->get_name());

    auto * method = new Method();

    if (cname->printname(cname->name).find("::") == std::string::npos) {
        // Handle constructors and destructors
        method->name = demangleName(sym->get_name());
        method->returnType = new TypePtr("", false);
    } else {
        method->name = cname->printname(cname->name);
        method->returnType = new TypePtr("int", (bool) cname->return_type.is_pointer); // TODO other types?
    }

    method->address = data.value;
    method->callType = cname->function_call; // inne wartości
    method->isStatic = cname->is_static;
    method->isVirtual = cname->is_virtual;
    method->accessibility = Accessibility::PUBLIC;

    if (method->isStatic)
        method->args.push_back(new Argument("self", new TypePtr(method->name, true)));

    int argNum = 1;
    for (auto &param : cname->parameters) {
        auto typePtr = getTypePtr(*cname, param);
        if (typePtr->type == "void") {
            delete(typePtr);
            continue;
        }

        std::string argName = "arg" + std::to_string(argNum++);

        auto * arg = new Argument(argName, typePtr);
        method->args.push_back(arg);
    }

    return method;
}

TypePtr * ELFExtractor::getTypePtr(retdec::demangler::cName &cname, retdec::demangler::cName::type_t &ttype) {
    std::string retvalue;

    switch (ttype.type) {
        case cName::ttype::TT_UNKNOWN:
            break;
        case cName::ttype::TT_PEXPR:
            retvalue += cname.printpexpr(ttype);
            break;
            //built-in type
        case cName::ttype::TT_BUILTIN:
            switch(ttype.b) {
                case cName::st_type::T_VOID:
                    retvalue += "void";
                    break;
                case cName::st_type::T_WCHAR:
                    retvalue += "wchar_t";
                    break;
                case cName::st_type::T_BOOL:
                    retvalue += "bool";
                    break;
                case cName::st_type::T_CHAR:
                    retvalue += "char";
                    break;
                case cName::st_type::T_SCHAR:
                    retvalue += "signed char";
                    break;
                case cName::st_type::T_UCHAR:
                    retvalue += "unsigned char";
                    break;
                case cName::st_type::T_SHORT:
                    retvalue += "short";
                    break;
                case cName::st_type::T_USHORT:
                    retvalue += "unsigned short";
                    break;
                case cName::st_type::T_INT:
                    retvalue += "int";
                    break;
                case cName::st_type::T_UINT:
                    retvalue += "unsigned int";
                    break;
                case cName::st_type::T_LONG:
                    retvalue += "long";
                    break;
                case cName::st_type::T_ULONG:
                    retvalue += "unsigned long";
                    break;
                case cName::st_type::T_LONGLONG:
                    retvalue += "long long";
                    break;
                case cName::st_type::T_ULONGLONG:
                    retvalue += "unsigned long long";
                    break;
                case cName::st_type::T_INT128:
                    retvalue += "__int128";
                    break;
                case cName::st_type::T_UINT128:
                    retvalue += "unsigned __int128";
                    break;
                case cName::st_type::T_FLOAT:
                    retvalue += "float";
                    break;
                case cName::st_type::T_DOUBLE:
                    retvalue += "double";
                    break;
                case cName::st_type::T_LONGDOUBLE:
                    retvalue += "long double";
                    break;
                case cName::st_type::T_FLOAT128:
                    retvalue += "__float128";
                    break;
                case cName::st_type::T_ELLIPSIS:
                    retvalue += "ellipsis";
                    break;
                case cName::st_type::T_DD:
                    retvalue += "IEEE 754r decimal floating point (64 bits)";
                    break;
                case cName::st_type::T_DE:
                    retvalue += "IEEE 754r decimal floating point (128 bits)";
                    break;
                case cName::st_type::T_DF:
                    retvalue += "IEEE 754r decimal floating point (32 bits)";
                    break;
                case cName::st_type::T_DH:
                    retvalue += "IEEE 754r half-precision floating point (16 bits)";
                    break;
                case cName::st_type::T_CHAR32:
                    retvalue += "char32_t";
                    break;
                case cName::st_type::T_CHAR16:
                    retvalue += "char16_t";
                    break;
                case cName::st_type::T_AUTO:
                    retvalue += "auto";
                    break;
                case cName::st_type::T_NULLPTR:
                    retvalue += "std::nullptr_t";
                    break;
                default:
                    break;
            }
            break;

            //number
        case cName::ttype::TT_NUM:
            retvalue += std::to_string(ttype.num);
            break;
            //named type
        case cName::ttype::TT_NAME:
            //print the name
            retvalue += cname.printname(ttype.n);
            allDependentClasses.push_back(cname.printname(ttype.n));
            break;
        default:
            break;
    }

    TypePtr * typePtr = new TypePtr(retvalue, ttype.type == cName::ttype::TT_NAME);
    typePtr->isBaseType = ttype.type == cName::ttype::TT_BUILTIN;

    return typePtr;
}

bool ELFExtractor::isDuplicated(Type * type, Method *method) {
    for (auto &m : type->allMethods) {
        if (m->address == method->address) {
            return true;
        }
    }

    return false;
}

std::string ELFExtractor::getTypeFromSize(int size) {
    switch (elf->get_hdr().ei_class) {
        case ::elf::elfclass::_32:
            switch (size) {
                case 1:
                    return "char";
                case 2:
                    return "short";
                case 8:
                    return "double"; // or long long
                case 4:
                default:
                    return "int";
            }
        case ::elf::elfclass::_64:
            switch (size) {
                case 1:
                    return "char";
                case 2:
                    return "short";
                case 8:
                    return "long"; // or pointer
                case 4:
                default:
                    return "int";
            }
    }
}

std::vector<Field *> ELFExtractor::getAllGlobalVariables() {
    std::vector<Field *> fields;

    for (auto sym : symtab) {
        auto &data = sym.get_data();
        auto demangled = demangleName(sym.get_name(), true);

        if (data.type() != ::elf::stt::object || data.value == 0 || (sym.get_name().rfind("_Z", 0) == 0 && sym.get_name().rfind("_ZN", 0) != 0)) {
            continue;
        }

        TypePtr * fieldType = new TypePtr(getTypeFromSize(data.size), false);

        auto name = !demangled.empty() ? demangled : sym.get_name();
        auto * field = new Field(name, fieldType, 0);
        field->isStatic = true;
        field->address = data.value;
        field->accessibility = Accessibility::PUBLIC;

        fields.push_back(field);
    }

    return fields;
}

}
}