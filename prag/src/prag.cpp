#include <iostream>
#include <string>
#include <set>
#include <sstream>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

#include "CLI11.hpp"
#include "ast.h"
#include "ast_parser.h"
#include "parser_registry.h"
#include "walker_registry.h"

int main(int argc, char* argv[])
{
    std::ios::sync_with_stdio(false);
    std::cerr.tie(&std::cout);

    const auto& parsers = bhw::ParserRegistry::getParserRegistry();
    const auto& walkers = bhw::WalkerRegistry::getWalkerRegistry();

    CLI::App app{"ASTrie - AST generator with dynamic walker flags"};

    std::string inputFile;
    std::string overrideExt;
    bool out_ast = false;
    bool out_src = false;
    bool out_all = false;
    std::set<std::string> outWalkers;

    // -------- Positional input file (optional, "-" for stdin) --------
    app.add_option("input", inputFile, "Input file to parse (use '-' for stdin)");

    // -------- Optional parser override --------
    auto extOption = app.add_option("--ext", overrideExt, "Override input parser (extension)");
    std::string parserList;
    for (const auto& p : parsers.getLangs())
        parserList += p + " ";
    extOption->description("Override input parser. Available: " + parserList);

    // -------- Standard output flags --------
    app.add_flag("--out-ast", out_ast, "Dump AST");
    app.add_flag("--out-src", out_src, "Dump source");
    app.add_flag("--out-all", out_all, "Generate all outputs (AST, source, all walkers)");

    // -------- Dynamic walker flags --------
    for (const auto& lang : walkers.getLangs())
    {
        std::string flag = "--out-" + lang;
        app.add_flag_callback(flag,
            [&outWalkers, lang]() { outWalkers.insert(lang); },
            "Output language: " + lang);
    }

    CLI11_PARSE(app, argc, argv);

    // -------- Determine source --------
    std::string source;

#ifdef _WIN32
    _setmode(_fileno(stdin), _O_BINARY); // binary stdin on Windows
#endif

    if (inputFile.empty() || inputFile == "-")
    {
        // Read from stdin
        std::ostringstream ss;
        ss << std::cin.rdbuf();
        source = ss.str();

        if (source.empty())
        {
            std::cerr << "Error: No input file provided and stdin is empty.\n";
            return 1;
        }
    }
    else
    {
        source = bhw::readFile(inputFile);
    }

    // -------- Determine parser extension --------
    std::string ext;
    if (!overrideExt.empty())
    {
        ext = overrideExt;
    }
    else if (!inputFile.empty() && inputFile != "-")
    {
        ext = bhw::getFileExtension(inputFile).substr(1);
    }
    else
    {
        // stdin with no --ext
        std::cerr << "Error: Must specify --ext when reading from stdin\n";
        return 1;
    }

    std::cerr << "Input Parser: " << ext << "\n";

    auto parser = parsers.create(ext);
    if (!parser.has_value())
    {
        std::cerr << "No Parser for " << ext << "\n";
        return 1;
    }

    // -------- Meta-flag: out-all enables everything --------
    if (out_all)
    {
        out_ast = true;
        out_src = true;
        outWalkers = walkers.getLangs();
    }

    if (!out_ast && !out_src && outWalkers.empty())
    {
        std::cerr << "Error: No output options specified.\n";
        return 1;
    }

    try
    {
        const auto ast = parser.value()->parseToAst(source);

        if (out_src)
        {
            std::cerr << "********* SRC **********\n";
            std::cerr << source << "\n";
        }

        if (out_ast)
        {
            std::cerr << "********* AST **********\n";
            std::cerr << ast.showAst();
        }

        // -------- Output walkers --------
        for (const auto& lang : outWalkers)
        {
            auto parser = parsers.create(ext);
            std::cerr << "********* " << lang << " *********\n";
            const auto w = walkers.create(lang);
            auto a = parser.value()->parseToAst(source);
            std::cout << w->walk(std::move(a)) << "\n";
        }
    }
    catch (std::runtime_error& e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}

