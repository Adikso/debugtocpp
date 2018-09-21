#include <sstream>
#include "../utils/json.hpp"
#include "JsonClassDumper.hpp"
#include "../utils/cxxopts.h"

using json = nlohmann::json;

std::vector<std::string> JsonClassDumper::dump(std::vector<Type *> types, DumpConfig config) {
    std::vector<std::string> out;

    if (config.toDirectory) {
        for (auto type : types) {
            out.push_back(dumpAsJsonObj(type).dump(config.indent, ' ', false));
        }
    } else {
        json js;

        int i = 0;
        for (auto type : types) {
            js[i++] = dumpAsJsonObj(type);
        }

        out.push_back(js.dump(config.indent, ' ', false));
    }

    return out;
}

std::string JsonClassDumper::dump(Type *cls, DumpConfig config) {
    return dumpAsJsonObj(cls).dump(config.indent, ' ', false);
}

json JsonClassDumper::dumpAsJsonObj(Type *cls) {
    json js;

    js["className"] = cls->name;
    if (!cls->baseTypes.empty()) {
        js["baseClass"] = cls->baseTypes[0]->name;
    }

    for (int i = 0; i < cls->fields.size(); i++) {
        auto * field = cls->fields[i];
        js["fields"][i]["name"] = field->name;
        js["fields"][i]["type"] = field->typePtr->type + (field->typePtr->isPointer ? " * " : "");
        js["fields"][i]["isStatic"] = field->isStatic;
        js["fields"][i]["offset"] = field->offset;
    }

    for (int i = 0; i < cls->allMethods.size(); i++) {
        auto * method = cls->allMethods[i];
        js["methods"][i]["name"] = method->name;
        js["methods"][i]["returnType"] = method->returnType->type + (method->returnType->isPointer ? " * " : "");
        js["methods"][i]["isStatic"] = method->isStatic;
        js["methods"][i]["isVariadic"] = method->isVariadic;
        js["methods"][i]["isVirtual"] = method->isVirtual;
        js["methods"][i]["address"] = method->address;
        js["methods"][i]["callType"] = callingConventionNames[method->callType];

        for (int j = 0; j < method->args.size(); j++) {
            Argument *arg = method->args[j];
            js["methods"][i]["args"][j]["name"] = arg->name;
            js["methods"][i]["args"][j]["type"] = arg->typePtr->type + (arg->typePtr->isPointer ? " * " : "");
        }
    }

    return js;
}
