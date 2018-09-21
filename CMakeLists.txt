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

        src/utils/cxxopts.h src/extractor/pdb/PDBExtractor.cpp src/extractor/pdb/PDBExtractor.hpp src/extractor/Extractor.hpp src/common/DebugTypes.hpp src/dumper/ClassDumper.hpp src/dumper/CodeClassDumper.cpp src/dumper/CodeClassDumper.hpp src/dumper/JsonClassDumper.cpp src/dumper/JsonClassDumper.hpp src/utils/json.hpp src/utils/utils.hpp src/extractor/dwarf/DWARFExtractor.cpp src/extractor/dwarf/DWARFExtractor.hpp src/debugextract.hpp)

add_executable(debugtocpp ${DEBUGTOCPP_SOURCES})
target_include_directories(debugtocpp PUBLIC retdec/include)
target_include_directories(debugtocpp PUBLIC retdec/src/pdbparser)
target_include_directories(debugtocpp PUBLIC retdec/src/demangler)
target_include_directories(debugtocpp PUBLIC retdec/src/utils)
target_include_directories(debugtocpp PUBLIC libelfin/elf)