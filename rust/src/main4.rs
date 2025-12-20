use std::collections::HashMap;

// -------------------------
// Language Info for C++ mapping
// -------------------------

#[derive(Debug)]
pub struct PrimitiveTypes {
    pub bool_: &'static str,
    pub string_: &'static str,
    pub int32_: &'static str,
    pub uint32_: &'static str,
    pub int64_: &'static str,
    pub uint64_: &'static str,
    pub float_: &'static str,
    pub double_: &'static str,
}

#[derive(Debug)]
pub struct StandardTypes {
    pub datetime_: &'static str,
}

#[derive(Debug)]
pub struct ComplexTypes {
    pub vec_: &'static str,
    pub option_: &'static str,
    pub tuple_: &'static str,
    pub set_: &'static str,
    pub map_: &'static str,
}

#[derive(Debug)]
pub struct NamingConventions {
    pub class_name: fn(&str) -> String,
    pub private_member: fn(&str) -> String,
    pub public_member: fn(&str) -> String,
    pub getter_name: fn(&str) -> String,
}

#[derive(Debug)]
pub struct LanguageInfo {
    pub name: &'static str,
    pub primitives: PrimitiveTypes,
    pub standard: StandardTypes,
    pub complex: ComplexTypes,
    pub naming: NamingConventions,
}

fn to_pascal_case(s: &str) -> String {
    s.split('_').map(|w| {
        let mut c = w.chars();
        match c.next() {
            None => String::new(),
            Some(f) => f.to_uppercase().collect::<String>() + c.as_str(),
        }
    }).collect()
}

fn to_snake_case(s: &str) -> String {
    let mut out = String::new();
    for (i, c) in s.chars().enumerate() {
        if c.is_uppercase() && i != 0 {
            out.push('_');
        }
        out.push(c.to_ascii_lowercase());
    }
    out
}

fn to_camel_case(s: &str) -> String {
    let mut s = s.to_string();
    if let Some(c) = s.get_mut(0..1) {
        c.make_ascii_lowercase();
    }
    s
}

fn register_cpp_language() -> LanguageInfo {
    LanguageInfo {
        name: "CPP26",
        primitives: PrimitiveTypes {
            bool_: "bool",
            string_: "std::string",
            int32_: "int32_t",
            uint32_: "uint32_t",
            int64_: "int64_t",
            uint64_: "uint64_t",
            float_: "float",
            double_: "double",
        },
        standard: StandardTypes {
            datetime_: "std::chrono::system_clock::time_point",
        },
        complex: ComplexTypes {
            vec_: "std::vector<%s>",
            option_: "std::optional<%s>",
            tuple_: "std::tuple<%s>",
            set_: "std::set<%s>",
            map_: "std::map<%s, %s>",
        },
        naming: NamingConventions {
            class_name: to_pascal_case,
            private_member: to_snake_case,
            public_member: to_camel_case,
            getter_name: to_pascal_case,
        },
    }
}

// -------------------------
// Meta / Field Attributes
// -------------------------

#[derive(Debug, Clone, Copy)]
pub enum FieldAttr {
    PrimaryKey,
    Serializable,
    ExcludeCompare,
    ExcludeString,
    ExcludeHash,
}

#[derive(Debug)]
pub struct FieldMeta {
    pub rust_type: &'static str,
    pub field_name: &'static str,
    pub table_col: &'static str,
    pub csv_col: &'static str,
    pub attrs: &'static [FieldAttr],
}

pub trait MetaTuple {
    fn table_name() -> &'static str;
    fn class_name() -> &'static str;
    fn fields() -> &'static [FieldMeta];
}

// -------------------------
// Macro to generate struct + meta
// -------------------------

macro_rules! meta_tuple_struct {
    (
        $struct_name:ident,
        $table_name:expr,
        {
            $( $field:ident : $ftype:ty, table_col = $table_col:expr, csv_col = $csv_col:expr, attrs = [ $( $attr:expr ),* $(,)? ] ),* $(,)?
        }
    ) => {
        #[derive(Debug, Clone)]
        pub struct $struct_name {
            $( pub $field: $ftype ),*
        }

        impl $struct_name {
            pub fn new($( $field: $ftype ),*) -> Self {
                Self { $( $field ),* }
            }

            $(
                pub fn $field(&self) -> &$ftype {
                    &self.$field
                }
            )*
        }

        impl MetaTuple for $struct_name {
            fn table_name() -> &'static str { $table_name }
            fn class_name() -> &'static str { stringify!($struct_name) }
            fn fields() -> &'static [FieldMeta] {
                &[
                    $(
                        FieldMeta {
                            rust_type: stringify!($ftype),
                            field_name: stringify!($field),
                            table_col: $table_col,
                            csv_col: $csv_col,
                            attrs: &[$( $attr ),*],
                        }
                    ),*
                ]
            }
        }
    };
}

// -------------------------
// Recursive type resolution
// -------------------------

fn cpp_type(lang: &LanguageInfo, rust_type: &str) -> String {
    let table: HashMap<&str, &str> = [
        ("i32", lang.primitives.int32_),
        ("u32", lang.primitives.uint32_),
        ("i64", lang.primitives.int64_),
        ("u64", lang.primitives.uint64_),
        ("bool", lang.primitives.bool_),
        ("f32", lang.primitives.float_),
        ("f64", lang.primitives.double_),
        ("String", lang.primitives.string_),
    ].into_iter().collect();

    if let Some(mapped) = table.get(rust_type) {
        mapped.to_string()
    } else if rust_type.starts_with("Vec<") && rust_type.ends_with(">") {
        let inner = &rust_type[4..rust_type.len()-1];
        lang.complex.vec_.replace("%s", &cpp_type(lang, inner))
    } else if rust_type.starts_with("Option<") && rust_type.ends_with(">") {
        let inner = &rust_type[7..rust_type.len()-1];
        lang.complex.option_.replace("%s", &cpp_type(lang, inner))
    } else {
        rust_type.to_string()
    }
}

// -------------------------
// Generate C++ meta fields
// -------------------------

fn generate_cpp_fields<T: MetaTuple>(lang: &LanguageInfo) {
    println!("Meta fields for {}", T::class_name());
    for f in T::fields() {
        let cpp_ty = cpp_type(lang, f.rust_type);
        let attrs: Vec<String> = f.attrs.iter().map(|a| format!("{:?}", a)).collect();
        println!("meta::Field<{}, {}, &{}::{}, nullptr, nullptr, {}>,",
                 T::class_name(),
                 cpp_ty,
                 T::class_name(),
                 f.field_name,
                 attrs.join(" | "));
    }
}

// -------------------------
// Example usage
// -------------------------

meta_tuple_struct!(
    Athlete,
    "athlete",
    {
        id: u64, table_col = "athlete_id", csv_col = "ID", attrs = [FieldAttr::PrimaryKey, FieldAttr::Serializable],
        name: String, table_col = "athlete_name", csv_col = "Name", attrs = [FieldAttr::Serializable],
        scores: Vec<i32>, table_col = "scores", csv_col = "Scores", attrs = [FieldAttr::Serializable]
    }
);

// -------------------------
// Main
// -------------------------

fn main() {
    let lang = register_cpp_language();

    // Access struct
    let a = Athlete::new(1, "Alice".to_string(), vec![10, 20, 30]);
    println!("{:?}", a);

    // Generate C++ meta fields
    generate_cpp_fields::<Athlete>(&lang);
}



