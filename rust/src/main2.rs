use std::fmt::Debug;
use paste::paste;

// Field attributes
#[derive(Debug, Clone, Copy)]
pub enum FieldAttr {
    PrimaryKey,
    Serializable,
    ExcludeCompare,
    ExcludeString,
    ExcludeHash,
}

// Metadata for a field
#[derive(Debug)]
pub struct FieldMeta {
    pub rust_type: &'static str,
    pub field_name: &'static str,
    pub table_col: &'static str,
    pub csv_col: &'static str,
    pub attrs: &'static [FieldAttr],
}

// Trait for MetaTuple
pub trait MetaTuple {
    fn table_name() -> &'static str;
    fn class_name() -> &'static str;
    fn fields() -> &'static [FieldMeta];
}

// Macro to define struct + metadata + constructors/getters/setters
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
            // Constructor
            pub fn new($( $field: $ftype ),*) -> Self {
                Self { $( $field ),* }
            }

            // Getters & Setters
            $(
                pub fn $field(&self) -> &$ftype { &self.$field }
                paste! {
                    pub fn [<set_ $field>](&mut self, val: $ftype) { self.$field = val; }
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

// Example usage
meta_tuple_struct!(
    Athlete,
    "athlete",
    {
        id: u64, table_col = "athlete_id", csv_col = "ID", attrs = [FieldAttr::PrimaryKey, FieldAttr::Serializable],
        name: String, table_col = "athlete_name", csv_col = "Name", attrs = [FieldAttr::Serializable],
        scores: Vec<i32>, table_col = "scores", csv_col = "Scores", attrs = []
    }
);

fn main() {
    println!("Table: {}", Athlete::table_name());
    println!("Class: {}", Athlete::class_name());
    for field in Athlete::fields() {
        println!(
            "Field {} (Rust type {}) -> table '{}', CSV '{}', attrs {:?}",
            field.field_name, field.rust_type, field.table_col, field.csv_col, field.attrs
        );
    }

    let mut a = Athlete::new(1, "Alice".to_string(), vec![10, 20]);
    println!("{:?}", a);

    a.set_scores(vec![30, 40]);
    println!("{:?}", a);

    println!("{:?}", a.scores());
}

