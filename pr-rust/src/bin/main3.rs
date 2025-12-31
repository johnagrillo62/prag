use std::fmt::Write;

// ============================================================================
// LANGUAGE INFO
// ============================================================================

#[derive(Debug)]
struct PrimitiveTypes {
    bool_: &'static str,
    string_: &'static str,
    int32_: &'static str,
    uint32_: &'static str,
    int64_: &'static str,
    uint64_: &'static str,
    float_: &'static str,
    double_: &'static str,
}

#[derive(Debug)]
struct StandardTypes {
    datetime_: &'static str,
}

#[derive(Debug)]
struct ComplexTypes {
    vec_: &'static str,      // template: std::vector<%s>
    option_: &'static str,   // template: std::optional<%s>
    tuple_: &'static str,    // template: std::tuple<%s>
    set_: &'static str,      // template: std::set<%s>
    map_: &'static str,      // template: std::map<%s, %s>
}

#[derive(Debug)]
struct NamingConventions {
    class_name: fn(&str) -> String,
    private_member: fn(&str) -> String,
    public_member: fn(&str) -> String,
    getter_name: fn(&str) -> String,
}

#[derive(Debug)]
struct LanguageInfo {
    name: &'static str,
    primitives: PrimitiveTypes,
    standard: StandardTypes,
    complex: ComplexTypes,
    naming: NamingConventions,
}

fn to_pascal_case(s: &str) -> String {
    s.split('_')
        .map(|p| {
            let mut c = p.chars();
            match c.next() {
                None => "".to_string(),
                Some(first) => first.to_uppercase().collect::<String>() + c.as_str(),
            }
        })
        .collect()
}

fn to_camel_case(s: &str) -> String {
    let mut pascal = to_pascal_case(s);
    let mut chars = pascal.chars();
    match chars.next() {
        None => "".to_string(),
        Some(first) => first.to_lowercase().collect::<String>() + chars.as_str(),
    }
}

fn to_snake_case(s: &str) -> String {
    let mut result = String::new();
    for (i, c) in s.chars().enumerate() {
        if c.is_uppercase() && i != 0 {
            result.push('_');
        }
        result.push(c.to_ascii_lowercase());
    }
    result
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

// ============================================================================
// MACRO STRUCT
// ============================================================================

macro_rules! macro_tuple {
    ($struct_name:ident { $($field_name:ident : $field_type:expr),* $(,)? }) => {
        struct $struct_name;

        impl $struct_name {
            fn fields() -> Vec<(&'static str, &'static str)> {
                vec![
                    $( (stringify!($field_name), $field_type) ),*
                ]
            }
        }
    };
}

// ============================================================================
// ATHLETE STRUCT
// ============================================================================

macro_tuple! {
    Athlete {
        athlete: "int64_t",
        team1: "std::optional<int64_t>",
        last: "std::optional<std::string>",
        first: "std::optional<std::string>",
        initial: "std::optional<std::string>",
        sex: "std::string",
        birth: "std::chrono::system_clock::time_point",
        age: "int16_t",
        id_no: "std::string",
        inactive: "bool"
    }
}

// ============================================================================
// GENERATE CPP STRUCT
// ============================================================================

fn generate_cpp_struct(struct_name: &str, lang: &LanguageInfo, fields: Vec<(&str, &str)>) -> String {
    let mut cpp = String::new();

    writeln!(&mut cpp, "#include <cstdint>").unwrap();
    writeln!(&mut cpp, "#include <string>").unwrap();
    writeln!(&mut cpp, "#include <optional>").unwrap();
    writeln!(&mut cpp, "#include <vector>").unwrap();
    writeln!(&mut cpp, "#include <tuple>").unwrap();
    writeln!(&mut cpp, "#include <chrono>").unwrap();

    writeln!(&mut cpp, "\nnamespace hytek {{\nnamespace tm {{").unwrap();
    writeln!(&mut cpp, "struct {} {{", struct_name).unwrap();

    // Fields
    for (name, cpp_type) in &fields {
        let private_name = (lang.naming.private_member)(name);
        writeln!(&mut cpp, "    {} {}{{}};", cpp_type, private_name).unwrap();
    }

    // Meta
    writeln!(&mut cpp, "\n    // clang-format off").unwrap();
    writeln!(&mut cpp, "    static constexpr auto fields = std::tuple<").unwrap();
    for (i, (name, cpp_type)) in fields.iter().enumerate() {
        let comma = if i == fields.len() - 1 { "" } else { "," };
        writeln!(
            &mut cpp,
            "        meta::Field<{}, {}, &{}::{}, nullptr, nullptr, meta::Prop::Serializable>{}",
            struct_name,
            cpp_type,
            struct_name,
            (lang.naming.private_member)(name),
            comma
        ).unwrap();
    }
    writeln!(&mut cpp, "    >{{}};").unwrap();

    writeln!(&mut cpp, "    static constexpr auto tableName = \"athlete\";").unwrap();
    let col_names: Vec<_> = fields.iter().map(|(f, _)| *f).collect();
    writeln!(
        &mut cpp,
        "    static constexpr auto query = \"SELECT {} FROM athlete\";",
        col_names.join(", ")
    ).unwrap();
    writeln!(&mut cpp, "    // clang-format on").unwrap();

    writeln!(&mut cpp, "}};").unwrap();
    writeln!(&mut cpp, "}} // namespace tm").unwrap();
    writeln!(&mut cpp, "}} // namespace hytek").unwrap();

    writeln!(&mut cpp, "namespace meta {{").unwrap();
    writeln!(
        &mut cpp,
        "template<>\nstruct MetaTuple<::hytek::tm::{}> {{",
        struct_name
    ).unwrap();
    writeln!(
        &mut cpp,
        "    static constexpr auto& fields = ::hytek::tm::{}::fields;",
        struct_name
    ).unwrap();
    writeln!(
        &mut cpp,
        "    static constexpr auto tableName = ::hytek::tm::{}::tableName;",
        struct_name
    ).unwrap();
    writeln!(
        &mut cpp,
        "    static constexpr auto query = ::hytek::tm::{}::query;",
        struct_name
    ).unwrap();
    writeln!(&mut cpp, "}};").unwrap();
    writeln!(&mut cpp, "}} // namespace meta").unwrap();

    cpp
}

// ============================================================================
// MAIN
// ============================================================================

fn main() {
    let lang = register_cpp_language();
    let cpp = generate_cpp_struct("Athlete", &lang, Athlete::fields());
    println!("{}", cpp);
}


