#include <sstream>
#include "CodeClassDumper.hpp"
#include "../utils/cxxopts.h"
#include "../utils/utils.hpp"

// Should be rewritten
namespace debugtocpp {

std::string CodeClassDumper::invalidCharacters = "`#():;\"'?";

std::vector<std::string> CodeClassDumper::dump(std::vector<Type *> types, DumpConfig config) {
    std::vector<std::string> out;

    out.reserve(types.size());
    for (auto type : types) {
        out.push_back(dump(type, config));
    }

    return out;
}

std::string CodeClassDumper::dump(Type *cls, DumpConfig config) {
    std::string indent = std::string(static_cast<unsigned long>(config.indent), ' ');

    std::stringstream out;
    out << "// Generated automatically by debugtocpp\n\n";

    auto compilableClassName = clearString(cls->name);

    if (config.addGuards) {
        out << "#ifndef " << compilableClassName.c_str() << "_H\n"
               "#define " << compilableClassName.c_str() << "_H\n\n";
    }

    if (config.addIncludesOrDeclarations) {
        for (auto& type : cls->dependentTypes) {
//            if (type == cls->name) { // TODO Constructors duplicated
//                continue;
//            }

            auto compilableDependentName = clearString(type);
            std::string classDisplayName = config.compilable ? compilableDependentName : type;

            // Add #include only for base class to avoid circular dependency
            if (!cls->baseTypes.empty() && type == cls->baseTypes[0]->name && !config.useOnlyForwardDeclarations) {
                out << "#include \"" << compilableDependentName.c_str() << ".hpp\"\n";
            } else {
                out << "class " << classDisplayName.c_str() << ";\n";
            }
        }

        if (cls->dependentTypes.size() > 1)
            out << "\n";
    }

    // Show original names for filtered class names
    if (compilableClassName != cls->name) {
        out << "// Original Name: " << cls->name << "\n";
    }

    std::string classDisplayName = config.compilable ? compilableClassName : cls->name;
    out << "class " << classDisplayName.c_str();

    // Show base class
    if (!cls->baseTypes.empty()) {
        auto compilableBaseClassName = clearString(cls->baseTypes[0]->name);
        std::string baseClassDisplayName = config.compilable ? compilableBaseClassName : cls->baseTypes[0]->name;

        out << " : public " << baseClassDisplayName.c_str();
    }

    out << " {\n";

    Accessibility currentAccesibility = Accessibility::NONE;

    for (const auto &field : cls->fields) {
        if (config.showAsPointers && field->address == 0) {
            continue;
        }

        if (field->accessibility != currentAccesibility) {
            out << accesibilityNames[field->accessibility] << ":\n";
            currentAccesibility = field->accessibility;
        }

        // [static] TYPE NAME;
        out << indent
            << (field->isStatic ? "static " : "")
            << printType(field->typePtr, config.compilable)
            << " "
            << (config.showAsPointers && field->isStatic ? "* " : "")
            << getName(field->name, cls) << ";\n";
    }


    // Empty line between fields and methods
    if (!cls->fields.empty() && !cls->fullyDefinedMethods.empty())
        out << "\n";

    for (auto method : (config.showAsPointers ? cls->fullyDefinedMethods : cls->allMethods)) {
        if (method->accessibility != currentAccesibility) {
            out << accesibilityNames[method->accessibility] << ":\n";
            currentAccesibility = method->accessibility;
        }

        std::string methodName = getName(method->name, cls);
        if (config.compilable) {
            methodName = clearString(methodName);
        }

        if (config.compilable && (methodName.find_first_of(invalidCharacters) !=
            std::string::npos || methodName == cls->name)) {
            // Comment out methods with special characters
            out << "//";
        }

        auto typeDisplayName = printType(method->returnType, config.compilable);

        if (config.showAsPointers) {
            if (methodName[0] == '~') {
                out << "//";
            }

            // static RETURNTYPE (*NAME)(
            out << indent << "static "
                << typeDisplayName << (!method->returnType->type.empty() ? " " : "") << "(*"
                << methodName.c_str() << ")(";
        } else {
            // virtual static RETURNTYPE (
            out << indent
                << (method->isVirtual ? "virtual " : "")
                << (method->isStatic ? "static " : "")
                << typeDisplayName << (!method->returnType->type.empty() ? " " : "")
                << methodName.c_str() << "(";
        }

        dumpMethodArgs(out, method, config.showAsPointers, config);

        out << ");\n";
    }

    out << "};\n";

    if (config.showAsPointers)
        dumpPointers(out, cls, config);

    if (config.addGuards) {
        out << "#endif\n\n";
    }

    return out.str();
}

void CodeClassDumper::dumpPointers(std::stringstream &out, Type *cls, DumpConfig config) {
    for (const auto &field : cls->fields) {
        if (config.showAsPointers && field->address == 0) {
            continue;
        }

        std::string typeDisplayName = printType(field->typePtr, config.compilable);
        std::string fieldName = config.showAsPointers ? field->name : getName(field->name, cls);

        // [static] TYPE NAME;
        out << typeDisplayName << " * "
            << fieldName << " = (" << typeDisplayName << "*) 0x"<< std::hex << field->address << ";\n";
    }

    out << "\n";

    // RETURNTYPE (*CLASSNAME::NAME)(ARGTYPE ARGNAME) = (RETURNTYPE (*)(ARGTYPE ARGNAME)) 0xADDRESS;
    for (auto method : cls->fullyDefinedMethods) {
        std::string methodName = getName(method->name, cls);

        if (config.compilable && (methodName.find_first_of(invalidCharacters) !=
            std::string::npos || methodName == cls->name)) {
            // Comment out methods with special characters
            out << "//";
        }

        if (methodName[0] == '~') {
            out << "//";
        }

        std::string returnTypeDisplayName = printType(method->returnType, config.compilable);

        out << returnTypeDisplayName << " (*" << cls->name << "::"
            << methodName << ")(";

        dumpMethodArgs(out, method, true, config);
        out << ") = (" << returnTypeDisplayName << " (*)(";
        dumpMethodArgs(out, method, true, config);
        out << ")) 0x" << std::hex << method->address << ";\n";
    }
}

void CodeClassDumper::dumpMethodArgs(std::stringstream &out, Method *method, bool pointers, DumpConfig config) {
    for (int i = 0; i < method->args.size(); i++) {
        Argument *arg = method->args[i];

        // Skip 'this' argument
        if (arg->name == "this") {
            if (pointers) {
                arg->name = "self";
            } else {
                continue;
            }
        }

        out << printType(arg->typePtr, config.compilable)
            << (!arg->name.empty() ? " " : "")
            << arg->name;

        // Add comma between every argument
        if (i != method->args.size() - 1) {
            out << ", ";
        }
    }
}

std::string CodeClassDumper::printType(TypePtr * typePtr, bool compilable) {
    std::string name = typePtr->type;
    if (compilable)
        name = clearString(name);

    return (typePtr->isConstant ? std::string("const ") : "") + name + (typePtr->isReference ? "&" : "") + (typePtr->isPointer ? " *" : "");
}

std::string CodeClassDumper::getName(std::string fullName, Type *cls) {
    std::string prefix = cls->name + "::";
    if (strncmp(fullName.c_str(), prefix.c_str(), prefix.size()) == 0) {
        return fullName.substr(cls->name.size() + 2);
    }

    return fullName;
}


}