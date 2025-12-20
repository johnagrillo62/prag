import (
    "hytek-go/common"
)

// Company represents the data model
type Company struct {
    Name string `json:"name" csv:"name"`
    Headquarters Address `json:"headquarters" csv:"headquarters"`
    Taxid string `json:"taxId" csv:"taxId"`
    Offices map[string]Address `json:"offices" csv:"offices"`
}

// MetaTuples returns field metadata with runtime values
func (obj *Company) MetaTuples() []common.MetaTuple {
    return []common.MetaTuple{
        {FieldName: "name", ColumnName: "name", TypeName: "string", Value: obj.Name},
        {FieldName: "headquarters", ColumnName: "headquarters", TypeName: "Address", Value: obj.Headquarters},
        {FieldName: "taxId", ColumnName: "taxId", TypeName: "string", Value: obj.Taxid},
        {FieldName: "offices", ColumnName: "offices", TypeName: "map[string]Address", Value: obj.Offices},
    }
}

func (*Company) TableName() string {
    return "company"
}

