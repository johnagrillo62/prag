import (
    "hytek-go/common"
)

// Contactinfo represents the data model
type Contactinfo struct {
    Email string `json:"email" csv:"email"`
    Phone string `json:"phone" csv:"phone"`
    Address Address `json:"address" csv:"address"`
    Previousaddresses []Address `json:"previousAddresses" csv:"previousAddresses"`
}

// MetaTuples returns field metadata with runtime values
func (obj *Contactinfo) MetaTuples() []common.MetaTuple {
    return []common.MetaTuple{
        {FieldName: "email", ColumnName: "email", TypeName: "string", Value: obj.Email},
        {FieldName: "phone", ColumnName: "phone", TypeName: "string", Value: obj.Phone},
        {FieldName: "address", ColumnName: "address", TypeName: "Address", Value: obj.Address},
        {FieldName: "previousAddresses", ColumnName: "previousAddresses", TypeName: "[]Address", Value: obj.Previousaddresses},
    }
}

func (*Contactinfo) TableName() string {
    return "contactinfo"
}

