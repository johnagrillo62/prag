use crate::meta::{FieldValue, FieldTuple};

#[derive(Debug, Clone, Default)]
pub struct Address {
    pub street: String,
    pub city: String,
    pub zipcode: String,
    pub country: String,
}

impl Address {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn table_name() -> &'static str {
        "address"
    }

    pub fn meta_tuples(&self) -> Vec<FieldTuple> {
        vec![
            FieldTuple {
                field_name: "street",
                column_name: "street",
                type_name: "String",
                value: FieldValue::String(&self.street),
            },
            FieldTuple {
                field_name: "city",
                column_name: "city",
                type_name: "String",
                value: FieldValue::String(&self.city),
            },
            FieldTuple {
                field_name: "zipcode",
                column_name: "zipCode",
                type_name: "String",
                value: FieldValue::String(&self.zipcode),
            },
            FieldTuple {
                field_name: "country",
                column_name: "country",
                type_name: "String",
                value: FieldValue::String(&self.country),
            },
        ]
    }
}
