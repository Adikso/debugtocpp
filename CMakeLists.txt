cmake_minimum_required(VERSION 3.12)
project(debugtocpp)

set(CMAKE_CXX_STANDARD 14)

set(DEBUGTOCPP_SOURCES
        src/debugextract.cpp
        retdec/src/pdbparser/pdb_file.cpp
        retdec/src/pdbparser/pdb_types.cpp
        retdec/src/pdbparser/pdb_symbols.cpp
        retdec/src/pdbparser/pdb_utils.cpp

        retdec/src/demangler/demangler.cpp
        retdec/src/demangler/demtools.cpp
        retdec/src/demangler/gparser.cpp
        retdec/src/demangler/igrams.cpp

        retdec/src/demangler/stgrammars/borlandll.cpp
        retdec/src/demangler/stgrammars/gccll.cpp
        retdec/src/demangler/stgrammars/msll.cpp

        retdec/src/utils/filesystem_path.cpp
        retdec/src/utils/string.cpp

        libelfin/elf/elf.cc
        libelfin/elf/mmap_loader.cc

        libelfin/dwarf/abbrev.cc
        libelfin/dwarf/attrs.cc
        libelfin/dwarf/cursor.cc
        libelfin/dwarf/data.hh
        libelfin/dwarf/die.cc
        libelfin/dwarf/die_str_map.cc
        libelfin/dwarf/dwarf.cc
        libelfin/dwarf/elf.cc
        libelfin/dwarf/expr.cc
        libelfin/dwarf/line.cc
        libelfin/dwarf/rangelist.cc
        libelfin/dwarf/value.cc

        src/extractor/dwarf/to_string.cc

        include/utils/cxxopts.h src/extractor/pdb/PDBExtractor.cpp include/extractor/pdb/PDBExtractor.hpp src/extractor/dwarf/DWARFExtractor.cpp include/extractor/dwarf/DWARFExtractor.hpp include/extractor/Extractor.hpp include/common/DebugTypes.hpp include/dumper/ClassDumper.hpp src/dumper/CodeClassDumper.cpp include/dumper/CodeClassDumper.hpp src/dumper/JsonClassDumper.cpp include/dumper/JsonClassDumper.hpp include/utils/json.hpp include/utils/utils.hpp src/extractor/elf/ELFExtractor.cpp include/extractor/elf/ELFExtractor.hpp include/debugextract.hpp src/common/Analyser.cpp include/common/Analyser.hpp)

add_executable(debugtocpp ${DEBUGTOCPP_SOURCES})
target_include_directories(debugtocpp PUBLIC include)

target_include_directories(debugtocpp PUBLIC retdec/include)
target_include_directories(debugtocpp PUBLIC retdec/src/pdbparser)
target_include_directories(debugtocpp PUBLIC retdec/src/demangler)
target_include_directories(debugtocpp PUBLIC retdec/src/utils)
target_include_directories(debugtocpp PUBLIC libelfin/elf)
target_include_directories(debugtocpp PUBLIC libelfin/dwarf)