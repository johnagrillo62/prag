import (
    "hytek-go/common"
)

// Project represents the data model
type Project struct {
    Name string `json:"name" csv:"name"`
    Description string `json:"description" csv:"description"`
    Tags []string `json:"tags" csv:"tags"`
}

// MetaTuples returns field metadata with runtime values
func (obj *Project) MetaTuples() []common.MetaTuple {
    return []common.MetaTuple{
        {FieldName: "name", ColumnName: "name", TypeName: "string", Value: obj.Name},
        {FieldName: "description", ColumnName: "description", TypeName: "string", Value: obj.Description},
        {FieldName: "tags", ColumnName: "tags", TypeName: "[]string", Value: obj.Tags},
    }
}

func (*Project) TableName() string {
    return "project"
}

