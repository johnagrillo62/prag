use std::collections::HashMap;
use std::fs;
use syn::{parse_file, Item, Type, TypePath, PathArguments, GenericArgument, Fields};

// --- Type registry for C++ ---
#[derive(Debug)]
pub struct TypeMapping {
    pub target: &'static str,
    pub default_value: &'static str,
}

#[derive(Debug)]
pub struct ComplexMapping {
    pub vector: &'static str,
    pub optional: &'static str,
    pub map: &'static str,
    pub tuple: &'static str,
    pub set: &'static str,
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

            // Recursively handle generics
            let inner_types: Vec<String> = match &segment.arguments {
                PathArguments::AngleBracketed(args) => args.args.iter().filter_map(|arg| {
                    if let GenericArgument::Type(inner_ty) = arg {
                        Some(rust_type_to_cpp(inner_ty, registry))
                    } else { None }
                }).collect(),
                _ => vec![],
            };

            match ident_str.as_str() {
                "Option" => registry.complex.optional.replace("{}", &inner_types.join(", ")),
                "Vec" => registry.complex.vector.replace("{}", &inner_types.join(", ")),
                "HashMap" => {
                    if inner_types.len() == 2 {
                        registry.complex.map
                            .replace("{}", &inner_types[0])
                            .replacen("{}", &inner_types[1], 1)
                    } else { "std::map<?, ?>".to_string() }
                },
                "HashSet" => registry.complex.set.replace("{}", &inner_types.join(", ")),
                _ => {
                    // Look up primitive types
                    if let Some(mapping) = registry.primitives.get(ident_str.as_str()) {
                        mapping.target.to_string()
                    } else {
                        ident_str.clone() // fallback for nested structs
                    }
                }
            }
        },
        _ => "<?>".to_string(),
    }
}


// --- Generate full C++ struct from Rust struct AST ---
fn generate_cpp_struct(struct_name: &str, fields: &Vec<(String, Type)>, registry: &LanguageTypeRegistry) -> String {
    let mut cpp_fields = vec![];

    for (name, ty) in fields {
        let cpp_type = rust_type_to_cpp(ty, registry);
        cpp_fields.push(format!("    {} {};", cpp_type, name));
    }

    format!("struct {} {{\n{}\n}};", struct_name, cpp_fields.join("\n"))
}

// --- Parse structs from file ---
fn parse_struct_fields(path: &str) -> Vec<(String, Vec<(String, Type)>)> {
    let content = fs::read_to_string(path).expect("Cannot read file");
    
    let ast = parse_file(&content).expect("Failed to parse file");
    println!("parsed");
    let mut structs = vec![];

    for item in ast.items {
        if let Item::Struct(s) = item {
            if let Fields::Named(fields_named) = s.fields {
                let fields_vec = fields_named.named.into_iter()
                    .map(|f| (f.ident.unwrap().to_string(), f.ty))
                    .collect();
                structs.push((s.ident.to_string(), fields_vec));
            }
        }
    }
    println!("Stucts");
    structs
}

// --- Main ---
fn main() {
    let registry = cpp_type_registry();
    let structs = parse_struct_fields("examples.rs");
    println!("hello");
    for (name, fields) in structs {
        println!("name");    
        let cpp_struct = generate_cpp_struct(&name, &fields, &registry);
        println!("{}", cpp_struct);
        println!();
    }
}
