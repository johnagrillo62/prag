use crate::meta::{FieldValue, FieldTuple};

#[derive(Debug, Clone, Default)]
pub struct Company {
    pub name: String,
    pub headquarters: Address,
    pub taxid: String,
    pub offices: HashMap<String, Address>,
}

impl Company {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn table_name() -> &'static str {
        "company"
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
                field_name: "headquarters",
                column_name: "headquarters",
                type_name: "Address",
                value: FieldValue::Dynamic(Box::new(self.headquarters)),
            },
            FieldTuple {
                field_name: "taxid",
                column_name: "taxId",
                type_name: "String",
                value: FieldValue::String(&self.taxid),
            },
            FieldTuple {
                field_name: "offices",
                column_name: "offices",
                type_name: "HashMap<String, Address>",
                value: FieldValue::Dynamic(Box::new(self.offices)),
            },
        ]
    }
}
