use std::collections::HashMap;
use std::fs;
use syn::{
    parse_file, Data, DeriveInput, Fields, GenericArgument, Item, PathArguments, Type, TypePath,
};

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
    ]
    .into_iter()
    .collect();

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
    if let Type::Path(TypePath { path, .. }) = ty {
        let segment = path.segments.last().unwrap();
        let ident_str = segment.ident.to_string();

        // Recursively handle generic arguments
        let inner_types: Vec<String> = match &segment.arguments {
            PathArguments::AngleBracketed(args) => args
                .args
                .iter()
                .filter_map(|arg| {
                    if let GenericArgument::Type(inner_ty) = arg {
                        Some(rust_type_to_cpp(inner_ty, registry))
                    } else {
                        None
                    }
                })
                .collect(),
            _ => vec![],
        };

        match ident_str.as_str() {
            "Option" => registry.complex.optional.replace("{}", &inner_types.join(", ")),
            "Vec" => registry.complex.vector.replace("{}", &inner_types.join(", ")),
            "HashMap" => {
                if inner_types.len() == 2 {
                    registry
                        .complex
                        .map
                        .replace("{}", &inner_types[0])
                        .replacen("{}", &inner_types[1], 1)
                } else {
                    "std::map<?, ?>".to_string()
                }
            }
            "HashSet" => registry.complex.set.replace("{}", &inner_types.join(", ")),
            primitive => {
                // Lookup primitive types in registry
                registry
                    .primitives
                    .get(primitive)
                    .map(|m| m.target.to_string())
                    .unwrap_or(primitive.to_string())
            }
        }
    } else {
        "<?>".to_string()
    }
}

// Generate C++ struct and tuple alias from Rust struct AST
fn generate_cpp_struct_and_tuple(input: &DeriveInput, registry: &LanguageTypeRegistry) -> (String, String) {
    let struct_name = &input.ident;
    let mut cpp_fields = vec![];
    let mut tuple_fields = vec![];

    if let Data::Struct(data) = &input.data {
        if let Fields::Named(fields_named) = &data.fields {
            for field in &fields_named.named {
                let field_name = field.ident.as_ref().unwrap().to_string();
                let cpp_type = rust_type_to_cpp(&field.ty, registry);
                cpp_fields.push(format!("    {} {};", cpp_type, field_name));
                tuple_fields.push(cpp_type);
            }
        }
    }

    let struct_def = format!("struct {} {{\n{}\n}};", struct_name, cpp_fields.join("\n"));
    let tuple_def = format!("using {}Tuple = std::tuple<{}>;", struct_name, tuple_fields.join(", "));

    (struct_def, tuple_def)
}

fn parse_structs_from_file(path: &str) -> Vec<DeriveInput> {
    let src = fs::read_to_string(path).expect("Cannot read file");
    let ast = parse_file(&src).expect("Failed to parse file");

    let mut structs = vec![];

    for item in ast.items {
        if let Item::Struct(item_struct) = item {
            let data_struct = syn::DataStruct {
                fields: item_struct.fields,
                semi_token: item_struct.semi_token,
                struct_token: item_struct.struct_token,
            };

            let derive_input = DeriveInput {
                attrs: item_struct.attrs,
                vis: item_struct.vis,
                ident: item_struct.ident,
                generics: item_struct.generics,
                data: Data::Struct(data_struct),
            };
            structs.push(derive_input);
        }
    }

    structs
}


// --- Main ---
fn main() {
    let registry = cpp_type_registry();

    let structs = parse_structs_from_file("examples.rs");

    println!("Parsed {} structs", structs.len());

    for s in structs {
        let (cpp_struct, cpp_tuple) = generate_cpp_struct_and_tuple(&s, &registry);
        println!("{}", cpp_struct);
        println!("{}", cpp_tuple);
        println!();
    }
}

