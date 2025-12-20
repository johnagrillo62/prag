use crate::meta::{FieldValue, FieldTuple};

#[derive(Debug, Clone, Default)]
pub struct Contactinfo {
    pub email: String,
    pub phone: String,
    pub address: Address,
    pub previousaddresses: Vec<Address>,
}

impl Contactinfo {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn table_name() -> &'static str {
        "contactinfo"
    }

    pub fn meta_tuples(&self) -> Vec<FieldTuple> {
        vec![
            FieldTuple {
                field_name: "email",
                column_name: "email",
                type_name: "String",
                value: FieldValue::String(&self.email),
            },
            FieldTuple {
                field_name: "phone",
                column_name: "phone",
                type_name: "String",
                value: FieldValue::String(&self.phone),
            },
            FieldTuple {
                field_name: "address",
                column_name: "address",
                type_name: "Address",
                value: FieldValue::Dynamic(Box::new(self.address)),
            },
            FieldTuple {
                field_name: "previousaddresses",
                column_name: "previousAddresses",
                type_name: "Vec<Address>",
                value: FieldValue::Dynamic(Box::new(self.previousaddresses.clone())),
            },
        ]
    }
}
