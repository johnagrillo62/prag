use serde_json::{json, Value};
use std::fs;
use syn::{
    parse_file, Data, DeriveInput, Fields, GenericArgument, Item, PathArguments, Type, TypePath,
};


// Convert Rust type to prag JSON type representation
fn rust_type_to_prag_json(ty: &Type) -> Value {
    if let Type::Path(TypePath { path, .. }) = ty {
        let segment = path.segments.last().unwrap();
        let ident_str = segment.ident.to_string();

        // Get inner types for generics
        let inner_types: Vec<Value> = match &segment.arguments {
            PathArguments::AngleBracketed(args) => args
                .args
                .iter()
                .filter_map(|arg| {
                    if let GenericArgument::Type(inner_ty) = arg {
                        Some(rust_type_to_prag_json(inner_ty))
                    } else {
                        None
                    }
                })
                .collect(),
            _ => vec![],
        };

        match ident_str.as_str() {
            "Option" => {
                json!({
                    "kind": "generic",
                    "name": "Option",
                    "args": inner_types
                })
            }
            "Vec" => {
                json!({
                    "kind": "generic",
                    "name": "Vec",
                    "args": inner_types
                })
            }
            "HashMap" => {
                json!({
                    "kind": "generic",
                    "name": "Map",
                    "args": inner_types
                })
            }
            "HashSet" => {
                json!({
                    "kind": "generic",
                    "name": "Set",
                    "args": inner_types
                })
            }
            primitive => {
                json!({
                    "kind": "primitive",
                    "name": primitive
                })
            }
        }
    } else {
        json!({
            "kind": "primitive",
            "name": "unknown"
        })
    }
}

// Generate prag JSON struct from Rust struct AST
fn generate_prag_json_struct(input: &DeriveInput) -> Value {
    let struct_name = input.ident.to_string();
    let mut fields = vec![];

    if let Data::Struct(data) = &input.data {
        if let Fields::Named(fields_named) = &data.fields {
            for field in &fields_named.named {
                let field_name = field.ident.as_ref().unwrap().to_string();
                let field_type = rust_type_to_prag_json(&field.ty);
                
                fields.push(json!({
                    "name": field_name,
                    "type": field_type
                }));
            }
        }
    }

    json!({
        "type": "Struct",
        "name": struct_name,
        "fields": fields
    })
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
    let structs = parse_structs_from_file("examples.rs");

    //println!("Parsed {} structs", structs.len());

    // Build prag JSON AST
    let mut items = vec![];
    
    for s in structs {
        let prag_struct = generate_prag_json_struct(&s);
        items.push(prag_struct);
    }

    // Create prag JSON AST module
    let prag_ast = json!({
        "type": "Module",
        "items": items
    });

    // Output as pretty JSON
    println!("{}", serde_json::to_string_pretty(&prag_ast).unwrap());
}

