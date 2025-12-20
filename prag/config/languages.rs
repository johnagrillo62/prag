// AUTO-GENERATED - DO NOT EDIT
use std::collections::HashMap;
use once_cell::sync::Lazy;

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum Language {
    Cpp26,
    Python,
    Rust,
    Typescript,
    Go,
}

pub struct PrimitiveType {
    pub type_name: &'static str,
    pub default_value: &'static str,
}

pub struct LanguageInfo {
    pub file_ext: &'static str,
    pub comment_style: &'static str,
    pub primitives: HashMap<&'static str, PrimitiveType>,
    pub generics: HashMap<&'static str, PrimitiveType>,
}

pub static LANGUAGE_REGISTRY: Lazy<HashMap<Language, LanguageInfo>> = Lazy::new(|| {
    let mut map = HashMap::new();

    map.insert(Language::Cpp26, LanguageInfo {
        file_ext: "hpp",
        comment_style: "//",
        primitives: [
            ("bool", PrimitiveType {
                type_name: "bool",
                default_value: "{}",
            }),
            ("int8", PrimitiveType {
                type_name: "int8_t",
                default_value: "{}",
            }),
            ("uint8", PrimitiveType {
                type_name: "uint8_t",
                default_value: "{}",
            }),
            ("int16", PrimitiveType {
                type_name: "int16_t",
                default_value: "{}",
            }),
            ("uint16", PrimitiveType {
                type_name: "uint16_t",
                default_value: "{}",
            }),
            ("int32", PrimitiveType {
                type_name: "int32_t",
                default_value: "{}",
            }),
            ("uint32", PrimitiveType {
                type_name: "uint32_t",
                default_value: "{}",
            }),
            ("int64", PrimitiveType {
                type_name: "int64_t",
                default_value: "{}",
            }),
            ("uint64", PrimitiveType {
                type_name: "uint64_t",
                default_value: "{}",
            }),
            ("float32", PrimitiveType {
                type_name: "float",
                default_value: "{}",
            }),
            ("float64", PrimitiveType {
                type_name: "double",
                default_value: "{}",
            }),
            ("string", PrimitiveType {
                type_name: "std::string",
                default_value: "{}",
            }),
            ("bytes", PrimitiveType {
                type_name: "std::vector<uint8_t>",
                default_value: "{}",
            }),
        ].into_iter().collect(),
        generics: [
            ("list", PrimitiveType {
                type_name: "std::vector<{0}>",
                default_value: "{}",
            }),
            ("map", PrimitiveType {
                type_name: "std::map<{0}, {1}>",
                default_value: "{}",
            }),
            ("set", PrimitiveType {
                type_name: "std::set<{0}>",
                default_value: "{}",
            }),
            ("optional", PrimitiveType {
                type_name: "std::optional<{0}>",
                default_value: "std::nullopt",
            }),
            ("tuple", PrimitiveType {
                type_name: "std::tuple<{...}>",
                default_value: "{}",
            }),
            ("variant", PrimitiveType {
                type_name: "std::variant<{...}>",
                default_value: "{}",
            }),
            ("unique_ptr", PrimitiveType {
                type_name: "std::unique_ptr<{0}>",
                default_value: "nullptr",
            }),
            ("shared_ptr", PrimitiveType {
                type_name: "std::shared_ptr<{0}>",
                default_value: "nullptr",
            }),
        ].into_iter().collect(),
    });

    map.insert(Language::Python, LanguageInfo {
        file_ext: "py",
        comment_style: "#",
        primitives: [
            ("bool", PrimitiveType {
                type_name: "bool",
                default_value: "False",
            }),
            ("int8", PrimitiveType {
                type_name: "int",
                default_value: "0",
            }),
            ("uint8", PrimitiveType {
                type_name: "int",
                default_value: "0",
            }),
            ("int16", PrimitiveType {
                type_name: "int",
                default_value: "0",
            }),
            ("uint16", PrimitiveType {
                type_name: "int",
                default_value: "0",
            }),
            ("int32", PrimitiveType {
                type_name: "int",
                default_value: "0",
            }),
            ("uint32", PrimitiveType {
                type_name: "int",
                default_value: "0",
            }),
            ("int64", PrimitiveType {
                type_name: "int",
                default_value: "0",
            }),
            ("uint64", PrimitiveType {
                type_name: "int",
                default_value: "0",
            }),
            ("float32", PrimitiveType {
                type_name: "float",
                default_value: "0.0",
            }),
            ("float64", PrimitiveType {
                type_name: "float",
                default_value: "0.0",
            }),
            ("string", PrimitiveType {
                type_name: "str",
                default_value: """",
            }),
            ("bytes", PrimitiveType {
                type_name: "bytes",
                default_value: "b''",
            }),
        ].into_iter().collect(),
        generics: [
            ("list", PrimitiveType {
                type_name: "list[{0}]",
                default_value: "[]",
            }),
            ("map", PrimitiveType {
                type_name: "dict[{0}, {1}]",
                default_value: "{}",
            }),
            ("set", PrimitiveType {
                type_name: "set[{0}]",
                default_value: "set()",
            }),
            ("optional", PrimitiveType {
                type_name: "Optional[{0}]",
                default_value: "None",
            }),
            ("tuple", PrimitiveType {
                type_name: "tuple[{...}]",
                default_value: "()",
            }),
            ("variant", PrimitiveType {
                type_name: "Union[{...}]",
                default_value: "None",
            }),
        ].into_iter().collect(),
    });

    map.insert(Language::Rust, LanguageInfo {
        file_ext: "rs",
        comment_style: "//",
        primitives: [
            ("bool", PrimitiveType {
                type_name: "bool",
                default_value: "false",
            }),
            ("int8", PrimitiveType {
                type_name: "i8",
                default_value: "0",
            }),
            ("uint8", PrimitiveType {
                type_name: "u8",
                default_value: "0",
            }),
            ("int16", PrimitiveType {
                type_name: "i16",
                default_value: "0",
            }),
            ("uint16", PrimitiveType {
                type_name: "u16",
                default_value: "0",
            }),
            ("int32", PrimitiveType {
                type_name: "i32",
                default_value: "0",
            }),
            ("uint32", PrimitiveType {
                type_name: "u32",
                default_value: "0",
            }),
            ("int64", PrimitiveType {
                type_name: "i64",
                default_value: "0",
            }),
            ("uint64", PrimitiveType {
                type_name: "u64",
                default_value: "0",
            }),
            ("float32", PrimitiveType {
                type_name: "f32",
                default_value: "0.0",
            }),
            ("float64", PrimitiveType {
                type_name: "f64",
                default_value: "0.0",
            }),
            ("string", PrimitiveType {
                type_name: "String",
                default_value: "String::new()",
            }),
            ("bytes", PrimitiveType {
                type_name: "Vec<u8>",
                default_value: "Vec::new()",
            }),
        ].into_iter().collect(),
        generics: [
            ("list", PrimitiveType {
                type_name: "Vec<{0}>",
                default_value: "Vec::new()",
            }),
            ("map", PrimitiveType {
                type_name: "HashMap<{0}, {1}>",
                default_value: "HashMap::new()",
            }),
            ("set", PrimitiveType {
                type_name: "HashSet<{0}>",
                default_value: "HashSet::new()",
            }),
            ("optional", PrimitiveType {
                type_name: "Option<{0}>",
                default_value: "None",
            }),
            ("tuple", PrimitiveType {
                type_name: "({...},)",
                default_value: "Default::default()",
            }),
            ("result", PrimitiveType {
                type_name: "Result<{0}, {1}>",
                default_value: "Err(Default::default())",
            }),
            ("box", PrimitiveType {
                type_name: "Box<{0}>",
                default_value: "Box::new(Default::default())",
            }),
            ("rc", PrimitiveType {
                type_name: "Rc<{0}>",
                default_value: "Rc::new(Default::default())",
            }),
            ("arc", PrimitiveType {
                type_name: "Arc<{0}>",
                default_value: "Arc::new(Default::default())",
            }),
        ].into_iter().collect(),
    });

    map.insert(Language::Typescript, LanguageInfo {
        file_ext: "ts",
        comment_style: "//",
        primitives: [
            ("bool", PrimitiveType {
                type_name: "boolean",
                default_value: "false",
            }),
            ("int8", PrimitiveType {
                type_name: "number",
                default_value: "0",
            }),
            ("uint8", PrimitiveType {
                type_name: "number",
                default_value: "0",
            }),
            ("int16", PrimitiveType {
                type_name: "number",
                default_value: "0",
            }),
            ("uint16", PrimitiveType {
                type_name: "number",
                default_value: "0",
            }),
            ("int32", PrimitiveType {
                type_name: "number",
                default_value: "0",
            }),
            ("uint32", PrimitiveType {
                type_name: "number",
                default_value: "0",
            }),
            ("int64", PrimitiveType {
                type_name: "bigint",
                default_value: "0n",
            }),
            ("uint64", PrimitiveType {
                type_name: "bigint",
                default_value: "0n",
            }),
            ("float32", PrimitiveType {
                type_name: "number",
                default_value: "0",
            }),
            ("float64", PrimitiveType {
                type_name: "number",
                default_value: "0",
            }),
            ("string", PrimitiveType {
                type_name: "string",
                default_value: """",
            }),
            ("bytes", PrimitiveType {
                type_name: "Uint8Array",
                default_value: "new Uint8Array()",
            }),
        ].into_iter().collect(),
        generics: [
            ("list", PrimitiveType {
                type_name: "Array<{0}>",
                default_value: "[]",
            }),
            ("map", PrimitiveType {
                type_name: "Map<{0}, {1}>",
                default_value: "new Map()",
            }),
            ("set", PrimitiveType {
                type_name: "Set<{0}>",
                default_value: "new Set()",
            }),
            ("optional", PrimitiveType {
                type_name: "{0} | null",
                default_value: "null",
            }),
            ("tuple", PrimitiveType {
                type_name: "[{...}]",
                default_value: "[]",
            }),
            ("union", PrimitiveType {
                type_name: "{...}",
                default_value: "null",
            }),
            ("promise", PrimitiveType {
                type_name: "Promise<{0}>",
                default_value: "Promise.resolve()",
            }),
        ].into_iter().collect(),
    });

    map.insert(Language::Go, LanguageInfo {
        file_ext: "go",
        comment_style: "//",
        primitives: [
            ("bool", PrimitiveType {
                type_name: "bool",
                default_value: "false",
            }),
            ("int8", PrimitiveType {
                type_name: "int8",
                default_value: "0",
            }),
            ("uint8", PrimitiveType {
                type_name: "uint8",
                default_value: "0",
            }),
            ("int16", PrimitiveType {
                type_name: "int16",
                default_value: "0",
            }),
            ("uint16", PrimitiveType {
                type_name: "uint16",
                default_value: "0",
            }),
            ("int32", PrimitiveType {
                type_name: "int32",
                default_value: "0",
            }),
            ("uint32", PrimitiveType {
                type_name: "uint32",
                default_value: "0",
            }),
            ("int64", PrimitiveType {
                type_name: "int64",
                default_value: "0",
            }),
            ("uint64", PrimitiveType {
                type_name: "uint64",
                default_value: "0",
            }),
            ("float32", PrimitiveType {
                type_name: "float32",
                default_value: "0.0",
            }),
            ("float64", PrimitiveType {
                type_name: "float64",
                default_value: "0.0",
            }),
            ("string", PrimitiveType {
                type_name: "string",
                default_value: """",
            }),
            ("bytes", PrimitiveType {
                type_name: "[]byte",
                default_value: "nil",
            }),
        ].into_iter().collect(),
        generics: [
            ("list", PrimitiveType {
                type_name: "[]{0}",
                default_value: "nil",
            }),
            ("map", PrimitiveType {
                type_name: "map[{0}]{1}",
                default_value: "make(map[{0}]{1})",
            }),
            ("set", PrimitiveType {
                type_name: "map[{0}]struct{}",
                default_value: "make(map[{0}]struct{})",
            }),
            ("optional", PrimitiveType {
                type_name: "*{0}",
                default_value: "nil",
            }),
            ("channel", PrimitiveType {
                type_name: "chan {0}",
                default_value: "make(chan {0})",
            }),
        ].into_iter().collect(),
    });

    map
});