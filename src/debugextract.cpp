#include <utility>

#include <utility>
#include <iostream>
#include <fstream>
#include <iostream>

#include "debugextract.hpp"

#include "common/Analyser.hpp"
#include "common/DebugTypes.hpp"
#include "dumper/CodeClassDumper.hpp"
#include "dumper/JsonClassDumper.hpp"
#include "extractor/pdb/PDBExtractor.hpp"
#include "extractor/elf/ELFExtractor.hpp"
#include "extractor/dwarf/DWARFExtractor.hpp"
#include "utils/cxxopts.h"
#include "utils/utils.hpp"

int main(int argc, char *argv[]) {
    cxxopts::Options options("debugtocpp", "Generate C++ classes from pdb/dwarf");
    options
            .positional_help("<pdb_file> [class_name]")
            .show_positional_help();

    options.allow_unrecognised_options().add_options()
            ("h,help", "Print help")
            ("b,base", "Image base", cxxopts::value<int>()->default_value("0x400000"))
            ("a,all", "Dump all classes (Used with --output dumps compilable header files to directory)")
            ("l,list", "List all classes")
            ("o,output", "Output path", cxxopts::value<std::string>())
            ("positional", "...", cxxopts::value<std::vector<std::string>>());
    options.parse_positional({"input", "class", "positional"});

    options.add_options("display")
            ("indent", "Output indentation level", cxxopts::value<int>()->default_value("2"))
            ("j,json", "Show output as json")
            ("no-compiler-generated", "Don't show compiler generated types/methods")
            ("c,compilable", "Modifies output so that it can be compiled")
            ("p,pointers", "Show members as pointers");

    auto args = options.parse(argc, argv);

    if (args.count("help") || !args.count("positional")) {
        std::cout << options.help({"", "display"}) << std::endl;
        return 0;
    }

    auto &v = args["positional"].as<std::vector<std::string>>();
    if ((v.size() < 2 && !(args.count("all") || args.count("list"))) || v.empty()) {
        std::cout << options.help({"", "display"}) << std::endl;
        return 1;
    }

    Extractor * extractor;
    try {
        extractor = getExtractorForFile(v[0], args["base"].as<int>());
    } catch (std::string &error){
        std::cout << error << std::endl;
        return 2;
    }

    DumpConfig config = argsToConfig(args);

    std::string outputPath;
    if (args.count("output")) {
        outputPath = args["output"].as<std::string>();

        // Remove trailing slash
        if (outputPath.back() == '/' || outputPath.back() == '\\') {
            outputPath.pop_back();
        }

        if (isDirectory(outputPath)) {
            config.toDirectory = true;
        } else {
            // Don't use #include when everything is in the same file
            config.useOnlyForwardDeclarations = true;
        }
    }

    Analyser analyser{config};

    if (args.count("list")) {
        list(extractor, analyser);
        return 0;
    }

    std::list<std::string> names;
    if (args.count("all") > 0) {
        names = extractor->getTypesList(false);
    } else {
        names = split(v[1], ',');
    }

    std::vector<Type *> types = extractor->getTypes(std::move(names));
    analyser.process(types);

    ClassDumper * dumper = config.json ? (ClassDumper *) new JsonClassDumper : new CodeClassDumper;
    std::vector<std::string> dumpOut = dumper->dump(std::move(types), config);

    if (config.toDirectory) {
        std::string extension = (config.json ? "json" : "hpp");

        for (int i = 0; i < dumpOut.size(); i++) {
            std::ofstream file;
            file.open(outputPath + "/" + clearString(types[i]->name) + "." + extension);

            file << dumpOut[i];
            file.close();
        }
    } else {
        std::string out = join(dumpOut, "\n");

        if (args.count("output")) {
            std::ofstream file;
            file.open(outputPath);

            file << out;
            file.close();
        } else {
            std::cout << out;
        }
    }

    return 0;
}

void list(Extractor * extractor, Analyser &analyser) {
    for (auto &type : extractor->getTypesList(false)) {
        if (!analyser.isCompilerGeneratedType(type)) {
            std::cout << type << std::endl;
        }
    }
}

Extractor * getExtractorForFile(const std::string &filename, int base) {
    auto * pdbExtractor = new PDBExtractor;
    auto * elfExtractor = new ELFExtractor;
    auto * dwarfExtractor = new DWARFExtractor;

    ExtractResult pdbExtractResult = pdbExtractor->load(filename, base);
    ExtractResult elfExtractResult = elfExtractor->load(filename, base);
    ExtractResult dwarfExtractResult = dwarfExtractor->load(filename, base);

    if (pdbExtractResult == ExtractResult::OK) {
        return pdbExtractor;
    }
    else if (dwarfExtractResult == ExtractResult::OK) {
        return dwarfExtractor;
    }
    else if (elfExtractResult == ExtractResult::OK) {
        return elfExtractor;
    }

    std::stringstream errorMessage;
    errorMessage << "Failed to load file: ";

    if (pdbExtractResult == ExtractResult::ERR_FILE_OPEN) {
        errorMessage << "File not found";
    } else if (dwarfExtractResult == ExtractResult::MISSING_DEBUG || elfExtractResult == ExtractResult::MISSING_DEBUG) {
        errorMessage << "File does not contain debug information";
    } else {
        errorMessage << "Unsupported format";
    }

    errorMessage << std::endl;
    throw errorMessage.str();
}

DumpConfig argsToConfig(const cxxopts::ParseResult &args) {
    DumpConfig config;
    config.indent = args["indent"].as<int>();
    config.json = args.count("json") > 0;
    config.showAsPointers = args.count("pointers") > 0;
    config.noCompilerGenerated = args.count("no-compiler-generated") > 0;

    config.compilable = args.count("compilable") > 0;
    config.addGuards = args.count("compilable") > 0;
    config.addIncludesOrDeclarations = args.count("compilable") > 0;
    config.useOnlyForwardDeclarations = args.count("compilable") == 0;

    return config;
}

