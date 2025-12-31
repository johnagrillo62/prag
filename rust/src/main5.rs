use std::collections::{HashMap, HashSet};

use syn::{parse_quote, Type, TypePath, PathArguments, GenericArgument};
use once_cell::sync::Lazy;
use quote::quote;

// --- Your type registry structs ---
#[derive(Debug)]
pub struct TypeMapping {
    pub target: &'static str,
    pub default_value: &'static str,
}

#[derive(Debug)]
pub struct ComplexMapping {
    pub vector: &'static str,   // e.g. "std::vector<{}>"
    pub optional: &'static str, // e.g. "std::optional<{}>"
    pub map: &'static str,      // e.g. "std::map<{}, {}>"
    pub tuple: &'static str,    // e.g. "std::tuple<{}>"
    pub set: &'static str,      // e.g. "std::set<{}>"
}

#[derive(Debug)]
pub struct StandardMapping {
    pub datetime: &'static str,
}

#[derive(Debug)]
pub struct LanguageTypeRegistry {
    pub primitives: HashMap<&'static str, TypeMapping>,
    pub complex: ComplexMapping,
    pub standard: StandardMapping,
}

pub fn cpp_type_registry() -> LanguageTypeRegistry {
    let primitives: HashMap<_, _> = vec![
        ("bool", TypeMapping { target: "bool", default_value: "{}" }),
        ("String", TypeMapping { target: "std::string", default_value: "{}" }),
        ("f32", TypeMapping { target: "float", default_value: "{}" }),
        ("f64", TypeMapping { target: "double", default_value: "{}" }),
        ("i8", TypeMapping { target: "int8_t", default_value: "{}" }),
        ("u8", TypeMapping { target: "uint8_t", default_value: "{}" }),
        ("i16", TypeMapping { target: "int16_t", default_value: "{}" }),
        ("u16", TypeMapping { target: "uint16_t", default_value: "{}" }),
        ("i32", TypeMapping { target: "int32_t", default_value: "{}" }),
        ("u32", TypeMapping { target: "uint32_t", default_value: "{}" }),
        ("i64", TypeMapping { target: "int64_t", default_value: "{}" }),
        ("u64", TypeMapping { target: "uint64_t", default_value: "{}" }),
    ].into_iter().collect();

    LanguageTypeRegistry {
        primitives,
        complex: ComplexMapping {
            vector: "std::vector<{}>",
            map: "std::map<{}, {}>",
            optional: "std::optional<{}>",
            tuple: "std::tuple<{}>",
            set: "std::set<{}>",
        },
        standard: StandardMapping {
            datetime: "std::chrono::system_clock::time_point",
        },
    }
}




fn rust_type_to_cpp(ty: &Type, registry: &LanguageTypeRegistry) -> String {
    match ty {
        Type::Path(TypePath { path, .. }) => {
            let segment = path.segments.last().unwrap();
            let ident_str = segment.ident.to_string();

            // Handle generic arguments recursively
            let inner_tokens: Vec<String> = match &segment.arguments {
                PathArguments::AngleBracketed(args) => args.args.iter().filter_map(|arg| {
                    if let GenericArgument::Type(inner_ty) = arg {
                        Some(rust_type_to_cpp(inner_ty, registry))
                    } else { None }
                }).collect(),
                _ => vec![],
            };

            match ident_str.as_str() {
                "Option" => registry.complex.optional.replace("{}", &inner_tokens.join(", ")),
                "Vec" => registry.complex.vector.replace("{}", &inner_tokens.join(", ")),
                "HashMap" => {
                    if inner_tokens.len() == 2 {
                        registry.complex.map
                            .replace("{}", &inner_tokens[0])
                            .replacen("{}", &inner_tokens[1], 1)
                    } else { "std::map<?>".to_string() }
                },
                "HashSet" => registry.complex.set.replace("{}", &inner_tokens.join(", ")),
                "String" => registry.primitives.get("String").unwrap().target.to_string(),
                "i8" | "i16" | "i32" | "i64" |
                "u8" | "u16" | "u32" | "u64" |
                "f32" | "f64" | "bool" => ident_str.to_string(),
                _ => ident_str, // fallback for nested structs
            }
        },
        _ => "<?>".to_string(),
    }
}




// --- Example ---
fn main() {
    let registry = cpp_type_registry();

    // Parse some Rust types with syn
    let ty: Type = parse_quote! { Option<Vec<i32>> };
    let ty2: Type = parse_quote! { HashMap<String, Option<i64>> };

    let cpp_type1 = rust_type_to_cpp(&ty, &registry);
    let cpp_type2 = rust_type_to_cpp(&ty2, &registry);

    println!("Rust: Option<Vec<i32>> -> C++: {}", cpp_type1);
    println!("Rust: HashMap<String, Option<i64>> -> C++: {}", cpp_type2);
}





