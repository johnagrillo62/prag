use std::any::type_name;

#[derive(Debug, Clone)]
pub enum FieldAttr {
    PrimaryKey,
    Serializable,
    // add more as needed
}

pub struct FieldMeta {
    pub name: &'static str,
    pub rust_type: &'static str,
    pub table_col: &'static str,
    pub csv_col: &'static str,
    pub attrs: &'static [FieldAttr],
}

pub trait MetaTuple {
    fn table_name() -> &'static str;
    fn class_name() -> &'static str;
    fn fields() -> Vec<FieldMeta>;
}

macro_rules! meta_tuple_struct {
    (
        $name:ident, table_name = $table_name:expr;
        $(
            $field_name:ident : $ftype:ty {
                table_col: $table_col:expr,
                csv_col: $csv_col:expr,
                attrs: [$($attr:expr),* $(,)?]
            }
        ),* $(,)?
    ) => {
        pub struct $name {
            $(pub $field_name: $ftype),*
        }

        impl MetaTuple for $name {
            fn table_name() -> &'static str { $table_name }
            fn class_name() -> &'static str { stringify!($name) }
            fn fields() -> Vec<FieldMeta> {
                vec![
                    $(
                        FieldMeta {
                            name: stringify!($field_name),
                            rust_type: type_name::<$ftype>(),
                            table_col: $table_col,
                            csv_col: $csv_col,
                            attrs: &[$($attr),*],
                        }
                    ),*
                ]
            }
        }
    };
}

// Example usage:
meta_tuple_struct!(
    Athlete, table_name = "athlete";
    id: u64 { table_col: "athlete_id", csv_col: "ID", attrs: [FieldAttr::PrimaryKey, FieldAttr::Serializable] },
    name: String { table_col: "athlete_name", csv_col: "Name", attrs: [FieldAttr::Serializable] },
    scores: Vec<i32> { table_col: "scores", csv_col: "Scores", attrs: [] }
);

fn main() {
    println!("Table: {}", Athlete::table_name());
    println!("Class: {}", Athlete::class_name());
    for f in Athlete::fields() {
        println!(
            "Field {} (Rust type {}) -> table '{}', CSV '{}', attrs {:?}",
            f.name, f.rust_type, f.table_col, f.csv_col, f.attrs
        );
    }
}



