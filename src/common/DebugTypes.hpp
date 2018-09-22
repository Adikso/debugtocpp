#include <utility>

#ifndef DEBUGTOCPP_DEBUGTYPES_HPP
#define DEBUGTOCPP_DEBUGTYPES_HPP

#include <string>
#include <utility>
#include <list>
#include <retdec/pdbparser/pdb_file.h>

using namespace retdec::pdbparser;

namespace debugtocpp {
namespace types {

// TODO: It is pdb only
static std::map<int, std::string> callingConventionNames = {
        {CV_CALL_NEAR_C,      "__cdecl"},
        {CV_CALL_FAR_C,       "__cdecl"},
        {CV_CALL_NEAR_PASCAL, "__pascal"},
        {CV_CALL_FAR_PASCAL,  "__pascal"},
        {CV_CALL_NEAR_FAST,   "__fastcall"},
        {CV_CALL_FAR_FAST,    "__fastcall"},
        {CV_CALL_NEAR_STD,    "__stdcall"},
        {CV_CALL_FAR_STD,     "__stdcall"},
        {CV_CALL_NEAR_SYS,    "__syscall"},
        {CV_CALL_FAR_SYS,     "__syscall"},
        {CV_CALL_THISCALL,    "__thiscall"},
        {CV_CALL_MIPSCALL,    "__mipscall"},
        {CV_CALL_GENERIC,     "__genericcall"},
        {CV_CALL_ALPHACALL,   "__alphacall"},
        {CV_CALL_PPCCALL,     "__ppccall"},
        {CV_CALL_SHCALL,      "__superhcall"},
        {CV_CALL_ARMCALL,     "__armcall"},
        {CV_CALL_AM33CALL,    "__am33call"},
        {CV_CALL_TRICALL,     "__tricall"},
        {CV_CALL_SH5CALL,     "__sh5call"},
        {CV_CALL_M32RCALL,    "__m32rcall"}
};

class TypePtr {
public:
    std::string type;
    bool isPointer = false;

    TypePtr(const std::string &type, bool isPointer) : type(type), isPointer(isPointer) {}

    TypePtr() {}
};

struct Argument {
    std::string name;
    TypePtr * typePtr;

    Argument(const std::string &name, TypePtr *type) : name(name), typePtr(type) {}

    Argument() {}
};

struct Field {
    std::string name;
    TypePtr * typePtr;
    int offset = 0;
    unsigned long address = 0;

    bool isStatic = false;

    Field(const std::string &name, TypePtr *type, int offset) : name(name), typePtr(type), offset(offset) {}

    Field() {}
};

class Method {
public:
    std::string name;
    std::string mangledName;
    TypePtr * returnType;
    unsigned long address = 0;
    int callType = 0;
    std::vector<Argument *> args;

    bool isStatic = false;
    bool isVariadic = false;
    bool isVirtual = false;
};

class Type {
public:
    std::string name;
    std::vector<Type *> baseTypes;
    std::vector<Field *> fields;
    std::vector<Method *> fullyDefinedMethods;
    std::vector<Method *> allMethods;
    std::list<std::string> dependentTypes;

    Type(std::string name) : name(std::move(name)) {}

    Type() {}
};

}
}

#endif //DEBUGTOCPP_DEBUGTYPES_HPP
