#include "parser_registry.h"

#include <string>

#include "avro_parser.h"
#include "capnp_parser.h"
#include "cpp_parser.h"
#include "csharp_parser.h"
#include "flatbuf_parser.h"
#include "fsharp_parser.h"
#include "go_parser.h"
#include "graphql_parser.h"
#include "haskell_parser.h"
#include "jsonschema_parser.h"
#include "mdb_parser.h"
#include "ocaml_parser.h"
#include "openapi_parser.h"
#include "protobuf_parser.h"
#include "python_parser.h"
#include "rust_parser.h"
#include "thrift_parser.h"
#include "typescript_parser.h"
#include "prag_parser.h"

namespace
{
template <typename ParserType> void registerParser(const std::string& ext)
{
    auto& reg = bhw::ParserRegistry::instance();
    reg.add(ext, []() { return std::make_unique<ParserType>(); });
}
} // namespace

auto bhw::ParserRegistry::getParserRegistry() -> bhw::ParserRegistry&
{
    static bool initialized = false;
    if (!initialized)
    {
        registerParser<AvroParser>("avsc");
        registerParser<CSharpParser>("cs");
        registerParser<CapnProtoParser>("capnp");
        registerParser<CppParser>("h");
        registerParser<FSharpParser>("fs");
        registerParser<FlatBufParser>("fbs");
        registerParser<GoParser>("go");
        registerParser<GraphQLParser>("graphql");
        registerParser<HaskellParser>("hs");
        registerParser<JSONSchemaParser>("jsonschema");
        registerParser<MdbParser>("mdb");
        registerParser<OCamlParser>("ml");
        registerParser<OpenApiParser>("openapi");
        registerParser<ProtoBufParser>("proto");
        registerParser<RustParser>("rs");
	registerParser<ThriftParser>("thrift");
	registerParser<TypeScriptParser>("ts");
	registerParser<PragParser>("prag");
	
    }
    return ParserRegistry::instance();
};
