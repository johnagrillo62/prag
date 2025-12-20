use crate::meta::{FieldValue, FieldTuple};

#[derive(Debug, Clone, Default)]
pub struct User {
    pub name: String,
    pub age: i32,
    pub id: String,
    pub data: String,
    pub contact: ContactInfo,
    pub employer: Company,
    pub projects: Vec<Project>,
    pub metadata: HashMap<String, String>,
    pub investments: HashMap<String, Company>,
    pub nested: HashMap<HashMap<String, String>, i32>,
}

impl User {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn table_name() -> &'static str {
        "user"
    }

    pub fn meta_tuples(&self) -> Vec<FieldTuple> {
        vec![
            FieldTuple {
                field_name: "name",
                column_name: "name",
                type_name: "String",
                value: FieldValue::String(&self.name),
            },
            FieldTuple {
                field_name: "age",
                column_name: "age",
                type_name: "i32",
                value: FieldValue::I32(self.age),
            },
            FieldTuple {
                field_name: "id",
                column_name: "id",
                type_name: "String",
                value: FieldValue::String(&self.id),
            },
            FieldTuple {
                field_name: "data",
                column_name: "data",
                type_name: "String",
                value: FieldValue::String(&self.data),
            },
            FieldTuple {
                field_name: "contact",
                column_name: "contact",
                type_name: "ContactInfo",
                value: FieldValue::Dynamic(Box::new(self.contact)),
            },
            FieldTuple {
                field_name: "employer",
                column_name: "employer",
                type_name: "Company",
                value: FieldValue::Dynamic(Box::new(self.employer)),
            },
            FieldTuple {
                field_name: "projects",
                column_name: "projects",
                type_name: "Vec<Project>",
                value: FieldValue::Dynamic(Box::new(self.projects.clone())),
            },
            FieldTuple {
                field_name: "metadata",
                column_name: "metadata",
                type_name: "HashMap<String, String>",
                value: FieldValue::Dynamic(Box::new(self.metadata)),
            },
            FieldTuple {
                field_name: "investments",
                column_name: "investments",
                type_name: "HashMap<String, Company>",
                value: FieldValue::Dynamic(Box::new(self.investments)),
            },
            FieldTuple {
                field_name: "nested",
                column_name: "nested",
                type_name: "HashMap<HashMap<String, String>, i32>",
                value: FieldValue::Dynamic(Box::new(self.nested)),
            },
        ]
    }
}
