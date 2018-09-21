#ifndef DEBUGTOCPP_DEBUGEXTRACT_HPP
#define DEBUGTOCPP_DEBUGEXTRACT_HPP

#include "utils/cxxopts.h"
#include "common/DebugTypes.hpp"
#include "extractor/pdb/PDBExtractor.hpp"
#include "extractor/dwarf/DWARFExtractor.hpp"
#include "dumper/ClassDumper.hpp"
#include "common/Analyser.hpp"

using namespace debugtocpp;
using namespace debugtocpp::pdb;
using namespace debugtocpp::types;
using namespace debugtocpp::dwarf;

DumpConfig argsToConfig(const cxxopts::ParseResult &args);
std::vector<Type *> getTypes(Extractor * extractor, Analyser &analyser, std::list<std::string> names);
std::vector<std::string> dump(std::vector<Type *> types, DumpConfig config);

#endif //DEBUGTOCPP_DEBUGEXTRACT_HPP
