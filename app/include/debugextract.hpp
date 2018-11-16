#ifndef DEBUGTOCPP_DEBUGEXTRACT_HPP
#define DEBUGTOCPP_DEBUGEXTRACT_HPP

#include "utils/cxxopts.h"
#include "common/DebugTypes.hpp"
#include "extractor/dwarf/DWARFExtractor.hpp"
#include "extractor/elf/ELFExtractor.hpp"
#include "extractor/pdb/PDBExtractor.hpp"
#include "dumper/ClassDumper.hpp"
#include "common/Analyser.hpp"

using namespace debugtocpp;
using namespace debugtocpp::types;
using namespace debugtocpp::elf;
using namespace debugtocpp::pdb;
using namespace debugtocpp::dwarf;

DumpConfig argsToConfig(const cxxopts::ParseResult &args);
Extractor * getExtractorForFile(const std::string &filename, int base);

void list(Extractor * extractor, Analyser &analyser);

#endif //DEBUGTOCPP_DEBUGEXTRACT_HPP
