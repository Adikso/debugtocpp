#include <algorithm>

#include "common/Analyser.hpp"

void Analyser::process(std::vector<Type *> &types) {
    for (auto &type : types) {
        if (!process(type)) {
            types.erase(std::remove(types.begin(), types.end(), type), types.end());
        }
    }
}

bool Analyser::process(Type *type) {
    if (config.noCompilerGenerated && isCompilerGenerated(type)) {
        return false;
    }

    auto it = std::begin(type->allMethods);

    while (it != std::end(type->allMethods)) {
        Method * method = *it.base();

        bool compilerGenerated = isCompilerGenerated(method);
        if (compilerGenerated) {
            method->isCompilerGenerated = true;
            if (config.noCompilerGenerated) {
                it = type->allMethods.erase(it);
            }
        }

        if (!compilerGenerated || !config.noCompilerGenerated) {
            ++it;
        }
    }

    return true;
}

bool Analyser::isCompilerGenerated(Type *type) {
    return isCompilerGeneratedType(type->name);
}

bool Analyser::isCompilerGeneratedType(std::string &name) {
    return name.rfind("std::", 0) == 0 || name[0] == '_';
}

bool Analyser::isCompilerGenerated(Method *method) {
    static std::vector<std::string> msvc = {
            "__vbaseDtor" , "__vecDelDtor", "__dflt_ctor_closure", "__delDtor",
            "__vec_ctor", "__vec_dtor", "__vec_ctor_vb", "__ehvec_ctor",
            "__ehvec_dtor", "__ehvec_ctor_vb", "__copy_ctor_closure", "__local_vftable_ctor_closure",
            "__placement_delete_closure", "__placement_arrayDelete_closure", "_man_vec_ctor",
            "__man_vec_dtor", "__ehvec_copy_ctor", "__ehvec_copy_ctor_vb", "`scalar deleting destructor'"
    };

    return method->isCompilerGenerated || method->name.rfind("std::", 0) == 0 || std::count(msvc.begin(), msvc.end(), method->name);
}


