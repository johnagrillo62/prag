#include "walker_registry.h"

#include "ast_walker.h"
#include "avro_walker.h"
#include "capnp_walker.h"
#include "cpp_walker.h"
#include "go_walker.h"
#include "java_walker.h"
#include "jsonschema_walker.h"
#include "openapi_walker.h"
#include "protobuf_walker.h"
#include "python_walker.h"
#include "rust_walker.h"
#include "zig_walker.h"
#include "csharp_walker.h"
#include "fsharp_walker.h"
#include "haskell_walker.h"
#include "ocaml_walker.h"

#include "prag_walker.h"

namespace
{
template <typename WalkerType> void registerWalker(const std::string& ext)
{
    auto& reg = bhw::WalkerRegistry::instance();
    reg.add(ext, []() { return std::make_unique<WalkerType>(); });
}
} // namespace

namespace bhw
{

auto bhw::WalkerRegistry::getWalkerRegistry() -> WalkerRegistry&
{
    static bool initialized = false;

    if (!initialized)
    {
	registerWalker<CSharpAstWalker>("cs");
	registerWalker<FSharpAstWalker>("fs");
	registerWalker<HaskellAstWalker>("hs");
	registerWalker<OCamlAstWalker>("ml");	
        registerWalker<AvroAstWalker>("avsc");
        registerWalker<CppWalker>("h");
        registerWalker<CapnProtoAstWalker>("capnp");
        registerWalker<GoAstWalker>("go");
        registerWalker<JSONSchemaAstWalker>("jsonschema");
        registerWalker<JavaAstWalker>("java");
        registerWalker<OpenApiAstWalker>("openapi");
        registerWalker<ProtoBufAstWalker>("proto");
        registerWalker<PythonAstWalker>("py");
        registerWalker<RustAstWalker>("rs");
        registerWalker<ZigAstWalker>("zig");
        registerWalker<PragAstWalker>("prag");
        initialized = true;
    }

    return WalkerRegistry::instance();
}
} // namespace bhw
