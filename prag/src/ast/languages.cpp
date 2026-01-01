#include "languages.h"

#include <map>
#include <string>

#include "ast.h"
#include "language_info.h"

namespace bhw
{
const std::map<Language, LanguageInfo>& getRegistry()
{
    static const std::map<Language, LanguageInfo> registry = {
        {
            {Language::Capnp,
             {.file_ext = "capnp",
              .comment_style = "#",
              .flattening = {.structs = FlatteningPolicy::Preserve,
                             .anonymous = AnonymousPolicy::Rename,
                             .enums = FlatteningPolicy::Preserve,
                             .oneofs = FlatteningPolicy::Preserve,
                             .variants = FlatteningPolicy::Preserve},
              .type_map =
                  {
                      // Primitives
                      {ReifiedTypeId::Bool, {"Bool", ""}},
                      {ReifiedTypeId::Int8, {"Int8", ""}},
                      {ReifiedTypeId::UInt8, {"UInt8", ""}},
                      {ReifiedTypeId::Int16, {"Int16", ""}},
                      {ReifiedTypeId::UInt16, {"UInt16", ""}},
                      {ReifiedTypeId::Int32, {"Int32", ""}},
                      {ReifiedTypeId::UInt32, {"UInt32", ""}},
                      {ReifiedTypeId::Int64, {"Int64", ""}},
                      {ReifiedTypeId::UInt64, {"UInt64", ""}},
                      {ReifiedTypeId::Float32, {"Float32", ""}},
                      {ReifiedTypeId::Float64, {"Float64", ""}},
                      {ReifiedTypeId::String, {"Text", ""}},
                      {ReifiedTypeId::Bytes, {"Data", ""}},

                      // Containers
                      {ReifiedTypeId::List, {"List({0})", ""}},
                      {ReifiedTypeId::Map, {"Data", ""}}, // Cap'n Proto doesn't have native maps
                      {ReifiedTypeId::Set, {"Data", ""}}, // No native sets
                      {ReifiedTypeId::Optional, {"{0}", ""}}, // No explicit optional
                  },
              .naming = {.struct_name = "PascalCase",
                         .field_name = "camelCase",
                         .constant = "SCREAMING_SNAKE_CASE",
                         .namespace_style = "lowercase",
                         .file_name = "snake_case"}}},

            {Language::Cpp26,
             {.file_ext = "h",
              .comment_style = "//",
              .flattening = {.structs = FlatteningPolicy::Preserve,
                             .anonymous = AnonymousPolicy::Preserve,
                             .enums = FlatteningPolicy::Preserve,
                             .oneofs = FlatteningPolicy::Preserve,
                             .variants = FlatteningPolicy::Preserve},
              .type_map =
                  {
                      // Primitives
                      {ReifiedTypeId::Bool, {"bool", "false"}},
                      {ReifiedTypeId::Int8, {"int8_t", "0"}},
                      {ReifiedTypeId::UInt8, {"uint8_t", "0"}},
                      {ReifiedTypeId::Int16, {"int16_t", "0"}},
                      {ReifiedTypeId::UInt16, {"uint16_t", "0"}},
                      {ReifiedTypeId::Int32, {"int32_t", "0"}},
                      {ReifiedTypeId::UInt32, {"uint32_t", "0"}},
                      {ReifiedTypeId::Int64, {"int64_t", "0"}},
                      {ReifiedTypeId::UInt64, {"uint64_t", "0"}},
                      {ReifiedTypeId::Float32, {"float", "0.0f"}},
                      {ReifiedTypeId::Float64, {"double", "0.0"}},
                      {ReifiedTypeId::String, {"std::string", "\"\""}},
                      {ReifiedTypeId::Bytes, {"std::vector<uint8_t>", "{}"}},
                      {ReifiedTypeId::Char, {"char", "'\\0'"}},

                      // Standard types
                      {ReifiedTypeId::DateTime,
                       {"std::chrono::system_clock::time_point",
                        "std::chrono::system_clock::now()"}},
                      {ReifiedTypeId::Date, {"std::chrono::year_month_day", "{}"}},
                      {ReifiedTypeId::Time, {"std::chrono::hh_mm_ss<std::chrono::seconds>", "{}"}},
                      {ReifiedTypeId::Duration, {"std::chrono::duration<int64_t>", "{}"}},
                      {ReifiedTypeId::UUID, {"std::array<uint8_t, 16>", "{}"}},
                      {ReifiedTypeId::Decimal, {"double", "0.0"}},
                      {ReifiedTypeId::URL, {"std::string", "\"\""}},
                      {ReifiedTypeId::Email, {"std::string", "\"\""}},

                      // Containers
                      {ReifiedTypeId::List, {"std::vector<{0}>", "{}"}},
                      {ReifiedTypeId::Map, {"std::map<{0}, {1}>", "{}"}},
                      {ReifiedTypeId::Set, {"std::set<{0}>", "{}"}},
                      {ReifiedTypeId::Optional, {"std::optional<{0}>", "std::nullopt"}},
                      {ReifiedTypeId::Tuple, {"std::tuple<{...}>", "{}"}},
                      {ReifiedTypeId::Variant, {"std::variant<{...}>", "{}"}},
                      {ReifiedTypeId::Pair, {"std::pair<{0}, {1}>", "{}"}},
                      {ReifiedTypeId::Array, {"std::array<{0}, {1}>", "{}"}},
                      {ReifiedTypeId::UnorderedMap, {"std::unordered_map<{0}, {1}>", "{}"}},
                      {ReifiedTypeId::UnorderedSet, {"std::unordered_set<{0}>", "{}"}},

                      // Ownership
                      {ReifiedTypeId::UniquePtr, {"std::unique_ptr<{0}>", "nullptr"}},
                      {ReifiedTypeId::SharedPtr, {"std::shared_ptr<{0}>", "nullptr"}},
                  },
              .naming = {.struct_name = "PascalCase",
                         .field_name = "snake_case",
                         .constant = "UPPER_SNAKE_CASE",
                         .namespace_style = "snake_case",
                         .file_name = "snake_case"}}},

            {Language::Python,
             {.file_ext = "py",
              .comment_style = "#",
              .flattening = {.structs = FlatteningPolicy::Preserve,
                             .anonymous = AnonymousPolicy::Preserve,
                             .enums = FlatteningPolicy::Preserve,
                             .oneofs = FlatteningPolicy::Flatten,
                             .variants = FlatteningPolicy::Preserve},
              .type_map =
                  {
                      // Primitives
                      {ReifiedTypeId::Bool, {"bool", "False"}},
                      {ReifiedTypeId::Int8, {"int", "0"}},
                      {ReifiedTypeId::UInt8, {"int", "0"}},
                      {ReifiedTypeId::Int16, {"int", "0"}},
                      {ReifiedTypeId::UInt16, {"int", "0"}},
                      {ReifiedTypeId::Int32, {"int", "0"}},
                      {ReifiedTypeId::UInt32, {"int", "0"}},
                      {ReifiedTypeId::Int64, {"int", "0"}},
                      {ReifiedTypeId::UInt64, {"int", "0"}},
                      {ReifiedTypeId::Float32, {"float", "0.0"}},
                      {ReifiedTypeId::Float64, {"float", "0.0"}},
                      {ReifiedTypeId::String, {"str", "\"\""}},
                      {ReifiedTypeId::Bytes, {"bytes", "b''"}},
                      {ReifiedTypeId::Char, {"str", "\"\""}},

                      // Standard types
                      {ReifiedTypeId::DateTime, {"datetime", "datetime.now()"}},
                      {ReifiedTypeId::Date, {"date", "date.today()"}},
                      {ReifiedTypeId::Time, {"time", "time()"}},
                      {ReifiedTypeId::Duration, {"timedelta", "timedelta()"}},
                      {ReifiedTypeId::UUID, {"UUID", "UUID()"}},
                      {ReifiedTypeId::Decimal, {"Decimal", "Decimal(0)"}},
                      {ReifiedTypeId::URL, {"str", "\"\""}},
                      {ReifiedTypeId::Email, {"str", "\"\""}},

                      // Containers
                      {ReifiedTypeId::List, {"list[{0}]", "[]"}},
                      {ReifiedTypeId::Map, {"dict[{0}, {1}]", "{}"}},
                      {ReifiedTypeId::Set, {"set[{0}]", "set()"}},
                      {ReifiedTypeId::Optional, {"Optional[{0}]", "None"}},
                      {ReifiedTypeId::Tuple, {"tuple[{...}]", "()"}},
                      {ReifiedTypeId::Variant, {"Union[{...}]", "None"}},
                      {ReifiedTypeId::Pair, {"tuple[{0}, {1}]", "()"}},
                  },
              .naming = {.struct_name = "PascalCase",
                         .field_name = "snake_case",
                         .constant = "SCREAMING_SNAKE_CASE",
                         .namespace_style = "snake_case",
                         .file_name = "snake_case"}}},

            {Language::Rust,
             {.file_ext = "rs",
              .comment_style = "//",
              .flattening = {.structs = FlatteningPolicy::Preserve,
                             .anonymous = AnonymousPolicy::Rename,
                             .enums = FlatteningPolicy::Preserve,
                             .oneofs = FlatteningPolicy::Flatten,
                             .variants = FlatteningPolicy::Preserve},
              .type_map =
                  {
                      // Primitives
                      {ReifiedTypeId::Bool, {"bool", "false"}},
                      {ReifiedTypeId::Int8, {"i8", "0"}},
                      {ReifiedTypeId::UInt8, {"u8", "0"}},
                      {ReifiedTypeId::Int16, {"i16", "0"}},
                      {ReifiedTypeId::UInt16, {"u16", "0"}},
                      {ReifiedTypeId::Int32, {"i32", "0"}},
                      {ReifiedTypeId::UInt32, {"u32", "0"}},
                      {ReifiedTypeId::Int64, {"i64", "0"}},
                      {ReifiedTypeId::UInt64, {"u64", "0"}},
                      {ReifiedTypeId::Float32, {"f32", "0.0"}},
                      {ReifiedTypeId::Float64, {"f64", "0.0"}},
                      {ReifiedTypeId::String, {"String", "String::new()"}},
                      {ReifiedTypeId::Bytes, {"Vec<u8>", "Vec::new()"}},
                      {ReifiedTypeId::Char, {"char", "'\\0'"}},

                      // Standard types
                      {ReifiedTypeId::DateTime, {"DateTime<Utc>", "Utc::now()"}},
                      {ReifiedTypeId::Duration, {"Duration", "Duration::default()"}},
                      {ReifiedTypeId::UUID, {"Uuid", "Uuid::nil()"}},

                      // Containers
                      {ReifiedTypeId::List, {"Vec<{0}>", "Vec::new()"}},
                      {ReifiedTypeId::Map, {"HashMap<{0}, {1}>", "HashMap::new()"}},
                      {ReifiedTypeId::Set, {"HashSet<{0}>", "HashSet::new()"}},
                      {ReifiedTypeId::Optional, {"Option<{0}>", "None"}},
                      {ReifiedTypeId::Tuple, {"({...},)", "Default::default()"}},
                      {ReifiedTypeId::Variant, {"Result<{0}, {1}>", "Err(Default::default())"}},

                      // Ownership
                      {ReifiedTypeId::UniquePtr, {"Box<{0}>", "Box::new(Default::default())"}},
                      {ReifiedTypeId::SharedPtr, {"Arc<{0}>", "Arc::new(Default::default())"}},
                  },
              .naming = {.struct_name = "PascalCase",
                         .field_name = "snake_case",
                         .constant = "SCREAMING_SNAKE_CASE",
                         .namespace_style = "snake_case",
                         .file_name = "snake_case"}}},

            {Language::Typescript,
             {.file_ext = "ts",
              .comment_style = "//",
              .flattening = {.structs = FlatteningPolicy::Preserve,
                             .anonymous = AnonymousPolicy::Rename,
                             .enums = FlatteningPolicy::Preserve,
                             .oneofs = FlatteningPolicy::Flatten,
                             .variants = FlatteningPolicy::Preserve},
              .type_map =
                  {
                      // Primitives
                      {ReifiedTypeId::Bool, {"boolean", "false"}},
                      {ReifiedTypeId::Int8, {"number", "0"}},
                      {ReifiedTypeId::UInt8, {"number", "0"}},
                      {ReifiedTypeId::Int16, {"number", "0"}},
                      {ReifiedTypeId::UInt16, {"number", "0"}},
                      {ReifiedTypeId::Int32, {"number", "0"}},
                      {ReifiedTypeId::UInt32, {"number", "0"}},
                      {ReifiedTypeId::Int64, {"bigint", "0n"}},
                      {ReifiedTypeId::UInt64, {"bigint", "0n"}},
                      {ReifiedTypeId::Float32, {"number", "0"}},
                      {ReifiedTypeId::Float64, {"number", "0"}},
                      {ReifiedTypeId::String, {"string", "\"\""}},
                      {ReifiedTypeId::Bytes, {"Uint8Array", "new Uint8Array()"}},
                      {ReifiedTypeId::Char, {"string", "\"\""}},

                      // Standard types
                      {ReifiedTypeId::DateTime, {"Date", "new Date()"}},
                      {ReifiedTypeId::Duration, {"number", "0"}},
                      {ReifiedTypeId::UUID, {"string", "\"\""}},

                      // Containers
                      {ReifiedTypeId::List, {"Array<{0}>", "[]"}},
                      {ReifiedTypeId::Map, {"Map<{0}, {1}>", "new Map()"}},
                      {ReifiedTypeId::Set, {"Set<{0}>", "new Set()"}},
                      {ReifiedTypeId::Optional, {"{0} | null", "null"}},
                      {ReifiedTypeId::Tuple, {"[{...}]", "[]"}},
                      {ReifiedTypeId::Variant, {"{...}", "null"}},
                  },
              .naming = {.struct_name = "PascalCase",
                         .field_name = "camelCase",
                         .constant = "SCREAMING_SNAKE_CASE",
                         .namespace_style = "PascalCase",
                         .file_name = "kebab-case"}}},

            {Language::Go,
             {.file_ext = "go",
              .comment_style = "//",
              .flattening = {.structs = FlatteningPolicy::Flatten,
                             .anonymous = AnonymousPolicy::Rename,
                             .enums = FlatteningPolicy::Preserve,
                             .oneofs = FlatteningPolicy::Flatten,
                             .variants = FlatteningPolicy::Preserve},
              .type_map =
                  {
                      // Primitives
                      {ReifiedTypeId::Bool, {"bool", "false"}},
                      {ReifiedTypeId::Int8, {"int8", "0"}},
                      {ReifiedTypeId::UInt8, {"uint8", "0"}},
                      {ReifiedTypeId::Int16, {"int16", "0"}},
                      {ReifiedTypeId::UInt16, {"uint16", "0"}},
                      {ReifiedTypeId::Int32, {"int32", "0"}},
                      {ReifiedTypeId::UInt32, {"uint32", "0"}},
                      {ReifiedTypeId::Int64, {"int64", "0"}},
                      {ReifiedTypeId::UInt64, {"uint64", "0"}},
                      {ReifiedTypeId::Float32, {"float32", "0.0"}},
                      {ReifiedTypeId::Float64, {"float64", "0.0"}},
                      {ReifiedTypeId::String, {"string", "\"\""}},
                      {ReifiedTypeId::Bytes, {"[]byte", "nil"}},

                      // Standard types
                      {ReifiedTypeId::DateTime, {"time.Time", "time.Now()"}},
                      {ReifiedTypeId::Duration, {"time.Duration", "0"}},
                      {ReifiedTypeId::UUID, {"uuid.UUID", "uuid.Nil"}},

                      // Containers
                      {ReifiedTypeId::List, {"[]{0}", "nil"}},
                      {ReifiedTypeId::Map, {"map[{0}]{1}", "make(map[{0}]{1})"}},
                      {ReifiedTypeId::Set, {"map[{0}]struct{}", "make(map[{0}]struct{})"}},
                      {ReifiedTypeId::Optional, {"*{0}", "nil"}},

                      {ReifiedTypeId::Variant, {"any", "nil"}},
                  },
              .naming = {.struct_name = "PascalCase",
                         .field_name = "PascalCase",
                         .constant = "PascalCase",
                         .namespace_style = "lowercase", // package names
                         .file_name = "snake_case"}}},

            {Language::Java,
             {.file_ext = "java",
              .comment_style = "//",
              .flattening = {.structs = FlatteningPolicy::Preserve,
                             .anonymous = AnonymousPolicy::Rename,
                             .enums = FlatteningPolicy::Preserve,
                             .oneofs = FlatteningPolicy::Preserve,
                             .variants = FlatteningPolicy::Preserve},
              .type_map =
                  {
                      // Primitives
                      {ReifiedTypeId::Bool, {"boolean", "false"}},
                      {ReifiedTypeId::Int8, {"byte", "0"}},
                      {ReifiedTypeId::UInt8, {"byte", "0"}},
                      {ReifiedTypeId::Int16, {"short", "0"}},
                      {ReifiedTypeId::UInt16, {"short", "0"}},
                      {ReifiedTypeId::Int32, {"int", "0"}},
                      {ReifiedTypeId::UInt32, {"int", "0"}},
                      {ReifiedTypeId::Int64, {"long", "0L"}},
                      {ReifiedTypeId::UInt64, {"long", "0L"}},
                      {ReifiedTypeId::Float32, {"float", "0.0f"}},
                      {ReifiedTypeId::Float64, {"double", "0.0"}},
                      {ReifiedTypeId::String, {"String", "\"\""}},
                      {ReifiedTypeId::Bytes, {"byte[]", "new byte[0]"}},
                      {ReifiedTypeId::Char, {"char", "'\\0'"}},

                      // Standard types
                      {ReifiedTypeId::DateTime, {"Instant", "Instant.now()"}},
                      {ReifiedTypeId::Date, {"LocalDate", "LocalDate.now()"}},
                      {ReifiedTypeId::Time, {"LocalTime", "LocalTime.now()"}},
                      {ReifiedTypeId::Duration, {"Duration", "Duration.ZERO"}},
                      {ReifiedTypeId::UUID, {"UUID", "UUID.randomUUID()"}},
                      {ReifiedTypeId::Decimal, {"BigDecimal", "BigDecimal.ZERO"}},
                      {ReifiedTypeId::URL, {"String", "\"\""}},
                      {ReifiedTypeId::Email, {"String", "\"\""}},

                      // Containers (boxed types for generics)
                      {ReifiedTypeId::List, {"List<{0}>", "new ArrayList<>()"}},
                      {ReifiedTypeId::Map, {"Map<{0}, {1}>", "new HashMap<>()"}},
                      {ReifiedTypeId::Set, {"Set<{0}>", "new HashSet<>()"}},
                      {ReifiedTypeId::Optional, {"Optional<{0}>", "Optional.empty()"}},
                      {ReifiedTypeId::Tuple, {"Pair<{0}, {1}>", "null"}},
                      {ReifiedTypeId::Variant, {"Object", "null"}},
                      {ReifiedTypeId::Array, {"{0}[]", "null"}},
                      {ReifiedTypeId::UnorderedMap, {"HashMap<{0}, {1}>", "new HashMap<>()"}},
                      {ReifiedTypeId::UnorderedSet, {"HashSet<{0}>", "new HashSet<>()"}},
                  },
              .naming = {.struct_name = "PascalCase",
                         .field_name = "camelCase",
                         .constant = "UPPER_SNAKE_CASE",
                         .namespace_style = "lowercase",
                         .file_name = "PascalCase"}}},

            {Language::Zig,
             {.file_ext = "zig",
              .comment_style = "//",
              .flattening = {.structs = FlatteningPolicy::Preserve,
                             .anonymous = AnonymousPolicy::Rename,
                             .enums = FlatteningPolicy::Preserve,
                             .oneofs = FlatteningPolicy::Preserve,
                             .variants = FlatteningPolicy::Preserve},
              .type_map =
                  {
                      // Primitives
                      {ReifiedTypeId::Bool, {"bool", "false"}},
                      {ReifiedTypeId::Int8, {"i8", "0"}},
                      {ReifiedTypeId::UInt8, {"u8", "0"}},
                      {ReifiedTypeId::Int16, {"i16", "0"}},
                      {ReifiedTypeId::UInt16, {"u16", "0"}},
                      {ReifiedTypeId::Int32, {"i32", "0"}},
                      {ReifiedTypeId::UInt32, {"u32", "0"}},
                      {ReifiedTypeId::Int64, {"i64", "0"}},
                      {ReifiedTypeId::UInt64, {"u64", "0"}},
                      {ReifiedTypeId::Float32, {"f32", "0.0"}},
                      {ReifiedTypeId::Float64, {"f64", "0.0"}},
                      {ReifiedTypeId::String, {"[]const u8", "\"\""}},
                      {ReifiedTypeId::Bytes, {"[]u8", "&[_]u8{}"}},
                      {ReifiedTypeId::Char, {"u8", "0"}},

                      // Standard types
                      {ReifiedTypeId::DateTime, {"i64", "0"}}, // Unix timestamp
                      {ReifiedTypeId::Duration, {"i64", "0"}}, // Nanoseconds
                      {ReifiedTypeId::UUID, {"[16]u8", "[_]u8{0} ** 16"}},
                      {ReifiedTypeId::Decimal, {"f64", "0.0"}},
                      {ReifiedTypeId::URL, {"[]const u8", "\"\""}},
                      {ReifiedTypeId::Email, {"[]const u8", "\"\""}},

                      // Containers
                      {ReifiedTypeId::List,
                       {"std.ArrayList({0})", "std.ArrayList({0}).init(allocator)"}},
                      {ReifiedTypeId::Map,
                       {"std.AutoHashMap({0}, {1})", "std.AutoHashMap({0}, {1}).init(allocator)"}},
                      {ReifiedTypeId::Set,
                       {"std.AutoHashMap({0}, void)",
                        "std.AutoHashMap({0}, void).init(allocator)"}},
                      {ReifiedTypeId::Optional, {"?{0}", "null"}},
                      {ReifiedTypeId::Tuple, {"struct { {0}, {1} }", ".{}"}},
                      {ReifiedTypeId::Variant, {"union(enum) { {0}, {1} }", "undefined"}},
                      {ReifiedTypeId::Array, {"[{1}]{0}", "[_]{0}{} ** {1}"}},

                      // Ownership
                      {ReifiedTypeId::UniquePtr, {"*{0}", "undefined"}},
                      {ReifiedTypeId::SharedPtr, {"*{0}", "undefined"}},
                  },
              .naming = {.struct_name = "PascalCase",
                         .field_name = "snake_case",
                         .constant = "SCREAMING_SNAKE_CASE",
                         .namespace_style = "snake_case",
                         .file_name = "snake_case"}}},

            {Language::CSharp,
             {.file_ext = "cs",
              .comment_style = "//",
              .flattening = {.structs = FlatteningPolicy::Preserve,
                             .anonymous = AnonymousPolicy::Rename,
                             .enums = FlatteningPolicy::Preserve,
                             .oneofs = FlatteningPolicy::Preserve,
                             .variants = FlatteningPolicy::Preserve},
              .type_map =
                  {
                      // Primitives
                      {ReifiedTypeId::Bool, {"bool", "false"}},
                      {ReifiedTypeId::Int8, {"sbyte", "0"}},
                      {ReifiedTypeId::UInt8, {"byte", "0"}},
                      {ReifiedTypeId::Int16, {"short", "0"}},
                      {ReifiedTypeId::UInt16, {"ushort", "0"}},
                      {ReifiedTypeId::Int32, {"int", "0"}},
                      {ReifiedTypeId::UInt32, {"uint", "0"}},
                      {ReifiedTypeId::Int64, {"long", "0L"}},
                      {ReifiedTypeId::UInt64, {"ulong", "0UL"}},
                      {ReifiedTypeId::Float32, {"float", "0.0f"}},
                      {ReifiedTypeId::Float64, {"double", "0.0"}},
                      {ReifiedTypeId::String, {"string", "\"\""}},
                      {ReifiedTypeId::Bytes, {"byte[]", "Array.Empty<byte>()"}},
                      {ReifiedTypeId::Char, {"char", "'\\0'"}},

                      // Standard types
                      {ReifiedTypeId::DateTime, {"DateTime", "DateTime.Now"}},
                      {ReifiedTypeId::Date, {"DateOnly", "DateOnly.FromDateTime(DateTime.Now)"}},
                      {ReifiedTypeId::Time, {"TimeOnly", "TimeOnly.FromDateTime(DateTime.Now)"}},
                      {ReifiedTypeId::Duration, {"TimeSpan", "TimeSpan.Zero"}},
                      {ReifiedTypeId::UUID, {"Guid", "Guid.Empty"}},
                      {ReifiedTypeId::Decimal, {"decimal", "0m"}},
                      {ReifiedTypeId::URL, {"string", "\"\""}},
                      {ReifiedTypeId::Email, {"string", "\"\""}},

                      // Containers
                      {ReifiedTypeId::List, {"List<{0}>", "new List<{0}>()"}},
                      {ReifiedTypeId::Map, {"Dictionary<{0}, {1}>", "new Dictionary<{0}, {1}>()"}},
                      {ReifiedTypeId::Set, {"HashSet<{0}>", "new HashSet<{0}>()"}},
                      {ReifiedTypeId::Optional, {"{0}?", "null"}},
                      {ReifiedTypeId::Tuple, {"({...})", "default"}},
                      {ReifiedTypeId::Variant, {"object", "null"}},
                      {ReifiedTypeId::Array, {"{0}[]", "Array.Empty<{0}>()"}},
                  },
              .naming = {.struct_name = "PascalCase",
                         .field_name = "PascalCase",
                         .constant = "PascalCase",
                         .namespace_style = "PascalCase",
                         .file_name = "PascalCase"}}},

            {Language::FSharp,
             {.file_ext = "fs",
              .comment_style = "//",
              .flattening = {.structs = FlatteningPolicy::Flatten,
                             .anonymous = AnonymousPolicy::Rename,
                             .enums = FlatteningPolicy::Preserve,
                             .oneofs = FlatteningPolicy::Preserve,
                             .variants = FlatteningPolicy::Preserve},
              .type_map =
                  {
                      // Primitives
                      {ReifiedTypeId::Bool, {"bool", "false"}},
                      {ReifiedTypeId::Int8, {"sbyte", "0y"}},
                      {ReifiedTypeId::UInt8, {"byte", "0uy"}},
                      {ReifiedTypeId::Int16, {"int16", "0s"}},
                      {ReifiedTypeId::UInt16, {"uint16", "0us"}},
                      {ReifiedTypeId::Int32, {"int", "0"}},
                      {ReifiedTypeId::UInt32, {"uint32", "0u"}},
                      {ReifiedTypeId::Int64, {"int64", "0L"}},
                      {ReifiedTypeId::UInt64, {"uint64", "0UL"}},
                      {ReifiedTypeId::Float32, {"float32", "0.0f"}},
                      {ReifiedTypeId::Float64, {"float", "0.0"}},
                      {ReifiedTypeId::String, {"string", "\"\""}},
                      {ReifiedTypeId::Bytes, {"byte[]", "[||]"}},
                      {ReifiedTypeId::Char, {"char", "'\\000'"}},

                      // Standard types
                      {ReifiedTypeId::DateTime, {"DateTime", "DateTime.Now"}},
                      {ReifiedTypeId::Date, {"DateOnly", "DateOnly.FromDateTime(DateTime.Now)"}},
                      {ReifiedTypeId::Time, {"TimeOnly", "TimeOnly.FromDateTime(DateTime.Now)"}},
                      {ReifiedTypeId::Duration, {"TimeSpan", "TimeSpan.Zero"}},
                      {ReifiedTypeId::UUID, {"Guid", "Guid.Empty"}},
                      {ReifiedTypeId::Decimal, {"decimal", "0m"}},

                      // Containers
                      {ReifiedTypeId::List, {"{0} list", "[]"}},
                      {ReifiedTypeId::Map, {"Map<{0}, {1}>", "Map.empty"}},
                      {ReifiedTypeId::Set, {"Set<{0}>", "Set.empty"}},
                      {ReifiedTypeId::Optional, {"{0} option", "None"}},
                      {ReifiedTypeId::Tuple, {"{0} * {1}", "()"}},
                      {ReifiedTypeId::Variant, {"obj", "null"}},
                      {ReifiedTypeId::Array, {"{0}[]", "[||]"}},
                  },
              .naming = {.struct_name = "PascalCase",
                         .field_name = "camelCase",
                         .constant = "PascalCase",
                         .namespace_style = "PascalCase",
                         .file_name = "PascalCase"}}},

            {Language::OCaml,
             {.file_ext = "ml",
              .comment_style = "(*",
              .flattening = {.structs = FlatteningPolicy::Preserve,
                             .anonymous = AnonymousPolicy::Rename,
                             .enums = FlatteningPolicy::Preserve,
                             .oneofs = FlatteningPolicy::Flatten,
                             .variants = FlatteningPolicy::Preserve},
              .type_map =
                  {
                      // Primitives
                      {ReifiedTypeId::Bool, {"bool", "false"}},
                      {ReifiedTypeId::Int8, {"int", "0"}},
                      {ReifiedTypeId::UInt8, {"int", "0"}},
                      {ReifiedTypeId::Int16, {"int", "0"}},
                      {ReifiedTypeId::UInt16, {"int", "0"}},
                      {ReifiedTypeId::Int32, {"int", "0"}},
                      {ReifiedTypeId::UInt32, {"int", "0"}},
                      {ReifiedTypeId::Int64, {"int64", "0L"}},
                      {ReifiedTypeId::UInt64, {"int64", "0L"}},
                      {ReifiedTypeId::Float32, {"float", "0.0"}},
                      {ReifiedTypeId::Float64, {"float", "0.0"}},
                      {ReifiedTypeId::String, {"string", "\"\""}},
                      {ReifiedTypeId::Bytes, {"bytes", "Bytes.empty"}},
                      {ReifiedTypeId::Char, {"char", "'\\000'"}},

                      // Standard types
                      {ReifiedTypeId::DateTime, {"float", "0.0"}}, // Unix timestamp
                      {ReifiedTypeId::Duration, {"float", "0.0"}},
                      {ReifiedTypeId::UUID, {"string", "\"\""}},

                      // Containers
                      {ReifiedTypeId::List, {"{0} list", "[]"}},
                      {ReifiedTypeId::Map, {"({0}, {1}) Map.t", "Map.empty"}},
                      {ReifiedTypeId::Set, {"{0} Set.t", "Set.empty"}},
                      {ReifiedTypeId::Optional, {"{0} option", "None"}},
                      {ReifiedTypeId::Tuple, {"{0} * {1}", "()"}},
                      {ReifiedTypeId::Variant, {"unit", "()"}},
                      {ReifiedTypeId::Array, {"{0} array", "[||]"}},
                  },
              .naming = {.struct_name = "snake_case",
                         .field_name = "snake_case",
                         .constant = "snake_case",
                         .namespace_style = "PascalCase",
                         .file_name = "snake_case"}}},

            {Language::Haskell,
             {.file_ext = "hs",
              .comment_style = "--",
              .flattening = {.structs = FlatteningPolicy::Preserve,
                             .anonymous = AnonymousPolicy::Rename,
                             .enums = FlatteningPolicy::Preserve,
                             .oneofs = FlatteningPolicy::Flatten,
                             .variants = FlatteningPolicy::Preserve},
              .type_map =
                  {
                      // Primitives
                      {ReifiedTypeId::Bool, {"Bool", "False"}},
                      {ReifiedTypeId::Int8, {"Int8", "0"}},
                      {ReifiedTypeId::UInt8, {"Word8", "0"}},
                      {ReifiedTypeId::Int16, {"Int16", "0"}},
                      {ReifiedTypeId::UInt16, {"Word16", "0"}},
                      {ReifiedTypeId::Int32, {"Int32", "0"}},
                      {ReifiedTypeId::UInt32, {"Word32", "0"}},
                      {ReifiedTypeId::Int64, {"Int64", "0"}},
                      {ReifiedTypeId::UInt64, {"Word64", "0"}},
                      {ReifiedTypeId::Float32, {"Float", "0.0"}},
                      {ReifiedTypeId::Float64, {"Double", "0.0"}},
                      {ReifiedTypeId::String, {"Text", "\"\""}},
                      {ReifiedTypeId::Bytes, {"ByteString", "BS.empty"}},
                      {ReifiedTypeId::Char, {"Char", "'\\0'"}},

                      // Standard types
                      {ReifiedTypeId::DateTime, {"UTCTime", "getCurrentTime"}},
                      {ReifiedTypeId::Duration, {"NominalDiffTime", "0"}},
                      {ReifiedTypeId::UUID, {"UUID", "nil"}},

                      // Containers
                      {ReifiedTypeId::List, {"[{0}]", "[]"}},
                      {ReifiedTypeId::Map, {"Map.Map {0} {1}", "Map.empty"}},
                      {ReifiedTypeId::Set, {"Set.Set {0}", "Set.empty"}},
                      {ReifiedTypeId::Optional, {"Maybe {0}", "Nothing"}},
                      {ReifiedTypeId::Tuple, {"({0}, {1})", "()"}},
                      {ReifiedTypeId::Variant, {"()", "()"}},
                      {ReifiedTypeId::Array, {"Vector {0}", "V.empty"}},
                  },
              .naming = {.struct_name = "PascalCase",
                         .field_name = "camelCase",
                         .constant = "camelCase",
                         .namespace_style = "PascalCase",
                         .file_name = "PascalCase"}}},

        }

    };

    return registry;
};

} // namespace bhw
