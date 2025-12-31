use std::fmt::{Debug, Formatter, Result};

// =======================
// Traits & Core Types
// =======================

pub trait MetaTuple {
    fn meta_tuples(&self) -> Vec<FieldMeta>;
}

#[derive(Clone)]
pub enum FieldValue<'a> {
    Primitive(&'a dyn Debug),
    Nested(&'a dyn MetaTuple),
    VecOfPrimitive(Vec<&'a dyn Debug>),
    VecOfNested(Vec<FieldValue<'a>>),
    OptionalPrimitive(Option<&'a dyn Debug>),
    OptionalNested(Option<&'a dyn MetaTuple>),
}

pub struct FieldMeta<'a> {
    pub field_name: &'a str,
    pub value: FieldValue<'a>,
}

// =======================
// Debug Implementations
// =======================

impl<'a> Debug for FieldValue<'a> {
    fn fmt(&self, f: &mut Formatter<'_>) -> Result {
        match self {
            FieldValue::Primitive(v) => write!(f, "Primitive({:?})", v),
            FieldValue::Nested(_) => write!(f, "Nested(<MetaTuple>)"),
            FieldValue::VecOfPrimitive(vec) => write!(f, "VecOfPrimitive({:?})", vec),
            FieldValue::VecOfNested(_) => write!(f, "VecOfNested(<MetaTuple>...)"),
            FieldValue::OptionalPrimitive(opt) => write!(f, "OptionalPrimitive({:?})", opt),
            FieldValue::OptionalNested(_) => write!(f, "OptionalNested(<MetaTuple>)"),
        }
    }
}

impl<'a> Debug for FieldMeta<'a> {
    fn fmt(&self, f: &mut Formatter<'_>) -> Result {
        write!(f, "{} = {:?}", self.field_name, self.value)
    }
}

// =======================
// Example Structs
// =======================

#[derive(Debug, Clone)]
pub struct Address {
    street: String,
    city: String,
    zip_code: u32,
}

impl MetaTuple for Address {
    fn meta_tuples(&self) -> Vec<FieldMeta> {
        vec![
            FieldMeta {
                field_name: "street",
                value: FieldValue::Primitive(&self.street),
            },
            FieldMeta {
                field_name: "city",
                value: FieldValue::Primitive(&self.city),
            },
            FieldMeta {
                field_name: "zip_code",
                value: FieldValue::Primitive(&self.zip_code),
            },
        ]
    }
}

#[derive(Debug)]
pub struct Person {
    id: u64,
    name: String,
    address: Address,
    tags: Vec<String>,
    spouse: Option<Box<Person>>, // Optional nested
}

impl MetaTuple for Person {
    fn meta_tuples(&self) -> Vec<FieldMeta> {
        vec![
            FieldMeta {
                field_name: "id",
                value: FieldValue::Primitive(&self.id),
            },
            FieldMeta {
                field_name: "name",
                value: FieldValue::Primitive(&self.name),
            },
            FieldMeta {
                field_name: "address",
                value: FieldValue::Nested(&self.address),
            },
            FieldMeta {
                field_name: "tags",
                value: FieldValue::VecOfPrimitive(self.tags.iter().map(|s| s as &dyn Debug).collect()),
            },
            FieldMeta {
                field_name: "spouse",
                value: FieldValue::OptionalNested(self.spouse.as_deref().map(|p| p as &dyn MetaTuple)),
            },
        ]
    }
}

// =======================
// Recursive Print Function
// =======================

fn print_meta_recursive(meta: &[FieldMeta], indent: usize) {
    let pad = "  ".repeat(indent);
    for field in meta {
        print!("{}{}: ", pad, field.field_name);
        match &field.value {
            FieldValue::Primitive(v) => println!("{:?}", v),
            FieldValue::Nested(n) => {
                println!("<Nested>");
                print_meta_recursive(&n.meta_tuples(), indent + 1);
            }
            FieldValue::VecOfPrimitive(vec) => {
                println!("Vec<{:?}>", vec);
            }
            FieldValue::VecOfNested(_) => {
                println!("<Vec<Nested>>"); // Could expand recursively if needed
            }
            FieldValue::OptionalPrimitive(opt) => println!("Optional({:?})", opt),
            FieldValue::OptionalNested(opt) => {
                if let Some(n) = opt {
                    println!("<Optional Nested>");
                    print_meta_recursive(&n.meta_tuples(), indent + 1);
                } else {
                    println!("None");
                }
            }
        }
    }
}

// =======================
// Main
// =======================

fn main() {
    let address = Address {
        street: "123 Main St".to_string(),
        city: "Springfield".to_string(),
        zip_code: 12345,
    };

    let spouse = Person {
        id: 2,
        name: "Jane Doe".to_string(),
        address: address.clone(),
        tags: vec!["partner".to_string()],
        spouse: None,
    };

    let person = Person {
        id: 1,
        name: "John Doe".to_string(),
        address,
        tags: vec!["developer".to_string(), "rustacean".to_string()],
        spouse: Some(Box::new(spouse)),
    };

    println!("=== Meta Tuples ===");
    print_meta_recursive(&person.meta_tuples(), 0);
}

