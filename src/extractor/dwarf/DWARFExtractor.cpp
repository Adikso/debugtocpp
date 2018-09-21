#include <fcntl.h>
#include <iostream>
#include "DWARFExtractor.hpp"
#include "../../utils/utils.hpp"

using namespace retdec::demangler;

namespace debugtocpp {
namespace dwarf {

ExtractResult debugtocpp::dwarf::DWARFExtractor::load(std::string filename, int image_base) {
    int fd = open(filename.c_str(), O_RDONLY);

    if (fd < 0) {
        return ExtractResult::ERR_FILE_OPEN;
    }

    try {
        elf = new elf::elf(elf::create_mmap_loader(fd));
    } catch (elf::format_error& e) {
        if (strcmp(e.what(), "bad ELF magic number") == 0) {
            return ExtractResult::INVALID_FILE;
        } else {
            return ExtractResult::UNSUPPORTED_VERSION;
        }
    }
    demangler = retdec::demangler::CDemangler::createGcc(); // TODO use other

    // Find symtab
    for (auto &section : elf->sections()) {
        if (section.get_hdr().type != elf::sht::symtab)
            continue;

        symtab = section.as_symtab();
    }

    return ExtractResult::OK;
}

Type *DWARFExtractor::getType(std::string name) {
    Type * type = new Type(name);

    for (auto sym : symtab) {
        auto &data = sym.get_data();
        auto cname = demangler->demangleToClass(sym.get_name());

        // Ignore symbols with different name than specified
        if (cname->name.empty() || cname->name[0].un != name) {
            continue;
        }

        switch (data.type()) {
            case elf::stt::func: {
                type->allMethods.push_back(getMethod(&sym));
                type->fullyDefinedMethods.push_back(getMethod(&sym));
                break;
            }
            case elf::stt::object: {
                std::string fieldName = cname->printname(cname->name);
                TypePtr * fieldType = new TypePtr("int", false);

                auto * field = new Field(fieldName, fieldType, 0);
                field->isStatic = true;
                field->address = data.value;

                type->fields.push_back(field);
                break;
            }
        }
    }

    return type;
}

std::list<std::string> DWARFExtractor::getTypesList(bool showStructs) {
    std::list<std::string> names;

    for (auto sym : symtab) {
        auto &data = sym.get_data();

        std::string typedefName = demangleTypedef(sym.get_name());
        if (typedefName.empty()) {
            continue;
        }

        names.emplace_back(typedefName);
    }

    names.sort();
    names.unique();

    return names;
}

Method *DWARFExtractor::getMethod(std::string name) {
    for (auto sym : symtab) {
        auto &data = sym.get_data();
        auto cname = demangler->demangleToClass(sym.get_name());

        if (cname->name.empty() || cname->name[0].un != name) { // Złe porównanie anzwy?
            continue;
        }

        switch (data.type()) {
            case elf::stt::func: {
                if (cname->printname(cname->name) == name) {
                    return getMethod(&sym);
                }
                break;
            }
        }
    }

    return nullptr;
}

Method *DWARFExtractor::getMethod(elf::sym *sym) {
    auto &data = sym->get_data();
    auto cname = demangler->demangleToClass(sym->get_name());

    auto * method = new Method();
    method->name = cname->printname(cname->name);
    method->returnType = new TypePtr("int", (bool) cname->return_type.is_pointer); // TODO other types?
    method->address = data.value;
    method->callType = cname->function_call; // inne wartości
    method->isStatic = cname->is_static;
    method->isVirtual = cname->is_virtual;

    if (method->isStatic)
        method->args.push_back(new Argument("self", new TypePtr(method->name, true)));

    int argNum = 1;
    for (auto &param : cname->parameters) {
        std::string typeName = getParameterTypeName(*cname, param);
        if (typeName == "void") {
            continue;
        }

        std::string argName = "arg" + std::to_string(argNum++);
        auto typePtr = new TypePtr(typeName, param.type == cName::ttype::TT_NAME);

        auto * arg = new Argument(argName, typePtr);
        method->args.push_back(arg);
    }

    return method;
}

std::string DWARFExtractor::getParameterTypeName(retdec::demangler::cName &cname, retdec::demangler::cName::type_t &ttype) {
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
            break;
        default:
            break;
    }

    return retvalue;
}

}
}