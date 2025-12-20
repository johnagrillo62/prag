import (
    "hytek-go/common"
)

// User represents the data model
type User struct {
    Name string `json:"name" csv:"name"`
    Age int32 `json:"age" csv:"age"`
    Id string `json:"id" csv:"id"`
    Data string `json:"data" csv:"data"`
    Contact ContactInfo `json:"contact" csv:"contact"`
    Employer Company `json:"employer" csv:"employer"`
    Projects []Project `json:"projects" csv:"projects"`
    Metadata map[string]string `json:"metadata" csv:"metadata"`
    Investments map[string]Company `json:"investments" csv:"investments"`
    Nested map[map[string]string]int32 `json:"nested" csv:"nested"`
}

// MetaTuples returns field metadata with runtime values
func (obj *User) MetaTuples() []common.MetaTuple {
    return []common.MetaTuple{
        {FieldName: "name", ColumnName: "name", TypeName: "string", Value: obj.Name},
        {FieldName: "age", ColumnName: "age", TypeName: "int32", Value: obj.Age},
        {FieldName: "id", ColumnName: "id", TypeName: "string", Value: obj.Id},
        {FieldName: "data", ColumnName: "data", TypeName: "string", Value: obj.Data},
        {FieldName: "contact", ColumnName: "contact", TypeName: "ContactInfo", Value: obj.Contact},
        {FieldName: "employer", ColumnName: "employer", TypeName: "Company", Value: obj.Employer},
        {FieldName: "projects", ColumnName: "projects", TypeName: "[]Project", Value: obj.Projects},
        {FieldName: "metadata", ColumnName: "metadata", TypeName: "map[string]string", Value: obj.Metadata},
        {FieldName: "investments", ColumnName: "investments", TypeName: "map[string]Company", Value: obj.Investments},
        {FieldName: "nested", ColumnName: "nested", TypeName: "map[map[string]string]int32", Value: obj.Nested},
    }
}

func (*User) TableName() string {
    return "user"
}

