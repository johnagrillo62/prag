use crate::meta::{FieldValue, FieldTuple};

#[derive(Debug, Clone, Default)]
pub struct Project {
    pub name: String,
    pub description: String,
    pub tags: Vec<String>,
}

impl Project {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn table_name() -> &'static str {
        "project"
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
                field_name: "description",
                column_name: "description",
                type_name: "String",
                value: FieldValue::String(&self.description),
            },
            FieldTuple {
                field_name: "tags",
                column_name: "tags",
                type_name: "Vec<String>",
                value: FieldValue::VecString(&self.tags),
            },
        ]
    }
}
