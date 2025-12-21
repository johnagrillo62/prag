// test_conversions.cpp - Query parser metadata directly
#include <filesystem>
#include <gtest/gtest.h>

#include "parser_registry.h"
#include "test_util.h"
#include "walker_registry.h"

#include <string>

static const auto& parsers = bhw::ParserRegistry::getParserRegistry();
static const auto& walkers = bhw::WalkerRegistry::getWalkerRegistry();


std::string makeSafeName(const std::string& fullPath, const std::string& baseDir) {
    // Get path relative to baseDir
    std::filesystem::path rel = std::filesystem::relative(fullPath, baseDir);

    std::string str = rel.string(); // e.g., "a/sample.proto"
    
    // Sanitize: replace non-alphanumeric with '_'
    for (char& c : str)
        if (!std::isalnum(static_cast<unsigned char>(c)))
            c = '_';
    return str;
}


bool compareBytes(const std::string& a, const std::string& b)
{
    size_t len = std::min(a.size(), b.size());
    for (size_t i = 0; i < len; ++i)
    {
        if (static_cast<unsigned char>(a[i]) != static_cast<unsigned char>(b[i]))
        {
            std::cerr << "Mismatch at byte " << i << ": 0x" << std::hex
                      << (unsigned int)(unsigned char)a[i] << " != 0x" << std::hex
                      << (unsigned int)(unsigned char)b[i] << std::dec << "\n";
            return false;
        }
    }

    if (a.size() != b.size())
    {
        std::cerr << "Files differ in size: " << a.size() << " vs " << b.size() << "\n";
        return false;
    }

    return true;
}

class RoundTripTest : public ::testing::Test
{
  public:
    std::string file;
    std::string inLang;
    std::string outLang;
    void TestBody() override
    {
        std::string src;
        std::string outLangSrc;
        std::string srcInput = bhw::test::readFile(file);

        try
        {
            auto srcParser = parsers.create(inLang);

            auto outLangAst = srcParser.value()->parseToAst(srcInput);
            auto outLangWalker = walkers.create(outLang);
            outLangSrc = outLangWalker->walk(std::move(outLangAst));
            auto outLangParser = parsers.create(outLang);
            auto outLangRoundAst = outLangParser.value()->parseToAst(outLangSrc);
            auto outLangRoundWalker = walkers.create(outLang);
            auto outLangRoundSrc = outLangWalker->walk(std::move(outLangRoundAst));

            std::string norm_input = bhw::test::normalize(outLangSrc);
            std::string norm_output = bhw::test::normalize(outLangRoundSrc);
            if (norm_input != norm_output)
            {
                bhw::test::showDetailedDiff(norm_input, norm_output);
                auto srcParser = parsers.create(inLang);
                auto outLangAst = srcParser.value()->parseToAst(srcInput);
                auto outLangParser = parsers.create(outLang);
                auto outLangRoundAst = outLangParser.value()->parseToAst(outLangSrc);
                
                std::cerr << "********* Input " << inLang << " *********" << inLang << "\n"
                          << bhw::test::printLines(srcInput) << "\n"
                          << "********* AST   *********" << "\n"
                          << outLangAst.showAst() << "\n"
                          << "********* Output " << outLang << " *********\n"
                          << bhw::test::printLines(outLangSrc) << "\n"
                          << "********* Output AST" << " *********\n"
                          << outLangRoundAst.showAst() << "\n"
                          << "********* Output Round" << " *********\n"
                          << outLangRoundSrc << "\n";


                     

                FAIL();
            }
        }
        catch (const std::runtime_error& e)
        {
            auto srcParser = parsers.create(inLang);
            auto outLangAst = srcParser.value()->parseToAst(srcInput);

            std::cerr << "Runtime error occurred during test: " << e.what() << "\n"
                      << "********* Input " << inLang << " *********" << inLang << "\n"
                      << bhw::test::printLines(srcInput) << "\n"
                      << "********* AST   *********" << "\n"
                      << outLangAst.showAst() << "********* Output " << outLang << " *********\n"
                      << bhw::test::printLines(outLangSrc) << "\n";

            FAIL();
        }
    };
};

class ParserTest : public ::testing::Test
{
  public:
    std::string file;
    std::string inLang;
    std::string walkerLang;

    void TestBody() override
    {
        try
        {
            auto srcInput = bhw::test::readFile(file);
            auto srcParser = parsers.create(inLang);
            auto outLangAst = srcParser.value()->parseToAst(srcInput);
            auto outLangWalker = walkers.create(walkerLang);
            auto outLangSrc = outLangWalker->walk(std::move(outLangAst));
        }
        catch (const std::runtime_error& e)
        {
            std::cerr << "Runtime error occurred during test: " << e.what() << std::endl;
            FAIL(); // This will fail the test with a message about the exception
        }
    }
};

auto sanitize = [](std::string s) {
  for (char& c : s)
    if (!std::isalnum(static_cast<unsigned char>(c)))
      c = '_';

  size_t start = s.find_first_not_of('_');
  if (start != std::string::npos)
    s = s.substr(start);
  return s;
 };

struct AutoRegister
{
    AutoRegister()
    {

        for (const auto& parserLang : parsers.getLangs())
        {

            for (const auto& walkerLang : walkers.getLangs())
            {
                auto path = std::string("../tests/") + parserLang + "/inputs/";
                auto files = bhw::test::getTestFiles(path, "." + parserLang);

                for (auto f : files)
                {
		  
                   auto name = parserLang + "_" + walkerLang + "_" + sanitize(f);

                    ::testing::RegisterTest("Parser",
                                            name.c_str(),
                                            nullptr,
                                            nullptr,
                                            __FILE__,
                                            __LINE__,
                                            [parserLang, walkerLang, f]() -> ParserTest*
                                            {
                                                auto* t = new ParserTest();
                                                t->file = f;
                                                t->inLang = parserLang;
                                                t->walkerLang = walkerLang;
                                                return t;
                                            });
                }

                if (parsers.has(walkerLang))
                {
                    for (auto f : files)
                    {
		      auto name = parserLang + "_" + walkerLang + "_" + sanitize(f);

                        ::testing::RegisterTest("RoundTrip",
                                                name.c_str(),
                                                nullptr,
                                                nullptr,
                                                __FILE__,
                                                __LINE__,
                                                [parserLang, walkerLang, f]() -> RoundTripTest*
                                                {
                                                    auto* t = new RoundTripTest();
                                                    t->file = f;
                                                    t->inLang = parserLang;
                                                    t->outLang = walkerLang;
                                                    return t;
                                                });
                    }
                }
            }
        }
    }
} g_auto;

