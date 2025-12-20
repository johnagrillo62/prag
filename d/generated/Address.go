import (
    "hytek-go/common"
)

// Address represents the data model
type Address struct {
    Street string `json:"street" csv:"street"`
    City string `json:"city" csv:"city"`
    Zipcode string `json:"zipCode" csv:"zipCode"`
    Country string `json:"country" csv:"country"`
}

// MetaTuples returns field metadata with runtime values
func (obj *Address) MetaTuples() []common.MetaTuple {
    return []common.MetaTuple{
        {FieldName: "street", ColumnName: "street", TypeName: "string", Value: obj.Street},
        {FieldName: "city", ColumnName: "city", TypeName: "string", Value: obj.City},
        {FieldName: "zipCode", ColumnName: "zipCode", TypeName: "string", Value: obj.Zipcode},
        {FieldName: "country", ColumnName: "country", TypeName: "string", Value: obj.Country},
    }
}

func (*Address) TableName() string {
    return "address"
}

