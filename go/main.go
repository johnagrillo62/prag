package main

import (
	"go/ast"
	"go/parser"
	"go/token"
)

// C++ AST Node Types
type CppNode interface {
	Accept(v CppVisitor)
}

type CppVisitor interface {
	VisitTranslationUnit(*CppTranslationUnit)
	VisitStruct(*CppStruct)
	VisitField(*CppField)
	VisitMethod(*CppMethod)
	VisitType(*CppType)
}

// Translation Unit (file)
type CppTranslationUnit struct {
	Includes []*CppInclude
	Structs  []*CppStruct
}

func (n *CppTranslationUnit) Accept(v CppVisitor) {
	v.VisitTranslationUnit(n)
}

// Include directive
type CppInclude struct {
	Path     string
	IsSystem bool // <> vs ""
}

// Struct definition
type CppStruct struct {
	Name        string
	Fields      []*CppField
	Methods     []*CppMethod
	Constructor *CppMethod
}

func (n *CppStruct) Accept(v CppVisitor) {
	v.VisitStruct(n)
}

// Field
type CppField struct {
	Name string
	Type *CppType
}

func (n *CppField) Accept(v CppVisitor) {
	v.VisitField(n)
}

// Method
type CppMethod struct {
	Name       string
	ReturnType *CppType
	Params     []*CppParam
	IsConst    bool
	Body       []CppStmt
}

func (n *CppMethod) Accept(v CppVisitor) {
	v.VisitMethod(n)
}

// Parameter
type CppParam struct {
	Name string
	Type *CppType
}

// Type representation
type CppType struct {
	Base      string
	IsPointer bool
	IsRef     bool
	IsConst   bool
	Template  *CppTemplate
}

func (n *CppType) Accept(v CppVisitor) {
	v.VisitType(n)
}

// Template type (vector<T>, optional<T>, etc)
type CppTemplate struct {
	Name string
	Args []*CppType
}

// Statement (for method bodies)
type CppStmt interface {
	cppStmt()
}

type CppReturn struct {
	Expr CppExpr
}

func (s *CppReturn) cppStmt() {}

type CppAssign struct {
	LHS CppExpr
	RHS CppExpr
}

func (s *CppAssign) cppStmt() {}

// Expression
type CppExpr interface {
	cppExpr()
}

type CppIdent struct {
	Name string
}

func (e *CppIdent) cppExpr() {}

type CppMemberAccess struct {
	Object CppExpr
	Member string
}

func (e *CppMemberAccess) cppExpr() {}

// ============================================================================
// GO AST → C++ AST TRANSFORMER
// ============================================================================

type GoToCppTransformer struct {
	unit *CppTranslationUnit
}

func NewGoToCppTransformer() *GoToCppTransformer {
	return &GoToCppTransformer{
		unit: &CppTranslationUnit{
			Includes: []*CppInclude{
				{Path: "string", IsSystem: true},
				{Path: "vector", IsSystem: true},
				{Path: "unordered_map", IsSystem: true},
				{Path: "optional", IsSystem: true},
				{Path: "cstdint", IsSystem: true},
			},
		},
	}
}

func (t *GoToCppTransformer) ParseGoSource(src string) error {
	fset := token.NewFileSet()
	f, err := parser.ParseFile(fset, "", src, parser.ParseComments)
	if err != nil {
		return err
	}

	ast.Inspect(f, func(n ast.Node) bool {
		switch x := n.(type) {
		case *ast.TypeSpec:
			if structType, ok := x.Type.(*ast.StructType); ok {
				cppStruct := t.transformStruct(x.Name.Name, structType)
				t.unit.Structs = append(t.unit.Structs, cppStruct)
			}
		}
		return true
	})

	return nil
}

func (t *GoToCppTransformer) transformStruct(name string, goStruct *ast.StructType) *CppStruct {
	cppStruct := &CppStruct{
		Name:    name,
		Fields:  []*CppField{},
		Methods: []*CppMethod{},
	}

	// Transform fields
	for _, field := range goStruct.Fields.List {
		for _, fieldName := range field.Names {
			cppField := &CppField{
				Name: fieldName.Name,
				Type: t.transformType(field.Type),
			}
			cppStruct.Fields = append(cppStruct.Fields, cppField)
		}
	}

	// Generate default constructor
	cppStruct.Constructor = &CppMethod{
		Name:       name,
		ReturnType: nil, // constructors have no return type
		Params:     []*CppParam{},
		Body:       []CppStmt{}, // = default
	}

	// Generate getters
	for _, field := range cppStruct.Fields {
		getter := &CppMethod{
			Name: "get_" + field.Name,
			ReturnType: &CppType{
				Base:     field.Type.Base,
				IsRef:    true,
				IsConst:  true,
				Template: field.Type.Template,
			},
			Params:  []*CppParam{},
			IsConst: true,
			Body: []CppStmt{
				&CppReturn{
					Expr: &CppIdent{Name: field.Name},
				},
			},
		}
		cppStruct.Methods = append(cppStruct.Methods, getter)

		// Generate setter
		setter := &CppMethod{
			Name:       "set_" + field.Name,
			ReturnType: &CppType{Base: "void"},
			Params: []*CppParam{
				{
					Name: "val",
					Type: &CppType{
						Base:     field.Type.Base,
						IsRef:    true,
						IsConst:  true,
						Template: field.Type.Template,
					},
				},
			},
			Body: []CppStmt{
				&CppAssign{
					LHS: &CppIdent{Name: field.Name},
					RHS: &CppIdent{Name: "val"},
				},
			},
		}
		cppStruct.Methods = append(cppStruct.Methods, setter)
	}

	return cppStruct
}

func (t *GoToCppTransformer) transformType(goType ast.Expr) *CppType {
	switch typ := goType.(type) {
	case *ast.Ident:
		return t.mapBasicType(typ.Name)

	case *ast.StarExpr:
		// Pointer → std::optional
		baseType := t.transformType(typ.X)
		return &CppType{
			Base: "std::optional",
			Template: &CppTemplate{
				Name: "optional",
				Args: []*CppType{baseType},
			},
		}

	case *ast.ArrayType:
		// Slice → std::vector
		elemType := t.transformType(typ.Elt)
		return &CppType{
			Base: "std::vector",
			Template: &CppTemplate{
				Name: "vector",
				Args: []*CppType{elemType},
			},
		}

	case *ast.MapType:
		// Map → std::unordered_map
		keyType := t.transformType(typ.Key)
		valType := t.transformType(typ.Value)
		return &CppType{
			Base: "std::unordered_map",
			Template: &CppTemplate{
				Name: "unordered_map",
				Args: []*CppType{keyType, valType},
			},
		}
	}

	return &CppType{Base: "unknown"}
}

func (t *GoToCppTransformer) mapBasicType(goType string) *CppType {
	typeMap := map[string]string{
		"int":     "int32_t",
		"int32":   "int32_t",
		"int64":   "int64_t",
		"uint":    "uint32_t",
		"uint32":  "uint32_t",
		"uint64":  "uint64_t",
		"float32": "float",
		"float64": "double",
		"string":  "std::string",
		"bool":    "bool",
		"byte":    "uint8_t",
	}

	if mapped, ok := typeMap[goType]; ok {
		return &CppType{Base: mapped}
	}

	return &CppType{Base: goType}
}

func (t *GoToCppTransformer) GetAST() *CppTranslationUnit {
	return t.unit
}

// ============================================================================
// C++ AST → CODE EMITTER (Visitor Pattern)
// ============================================================================

type CppCodeEmitter struct {
	output []byte
	indent int
}

func (e *CppCodeEmitter) emit(data []byte) {
	e.output = append(e.output, data...)
}

func (e *CppCodeEmitter) emitIndent() {
	for i := 0; i < e.indent; i++ {
		e.emit([]byte("    "))
	}
}

func (e *CppCodeEmitter) VisitTranslationUnit(node *CppTranslationUnit) {
	// Emit includes
	e.emit([]byte("#pragma once\n\n"))
	for _, inc := range node.Includes {
		if inc.IsSystem {
			e.emit([]byte("#include <"))
			e.emit([]byte(inc.Path))
			e.emit([]byte(">\n"))
		} else {
			e.emit([]byte("#include \""))
			e.emit([]byte(inc.Path))
			e.emit([]byte("\"\n"))
		}
	}
	e.emit([]byte("\n"))

	// Emit structs
	for _, s := range node.Structs {
		s.Accept(e)
	}
}

func (e *CppCodeEmitter) VisitStruct(node *CppStruct) {
	e.emit([]byte("struct "))
	e.emit([]byte(node.Name))
	e.emit([]byte(" {\n"))
	e.indent++

	// Fields
	for _, field := range node.Fields {
		field.Accept(e)
	}

	e.emit([]byte("\n"))

	// Constructor
	if node.Constructor != nil {
		e.emitIndent()
		e.emit([]byte(node.Name))
		e.emit([]byte("() = default;\n\n"))
	}

	// Methods
	for _, method := range node.Methods {
		method.Accept(e)
	}

	e.indent--
	e.emit([]byte("};\n\n"))
}

func (e *CppCodeEmitter) VisitField(node *CppField) {
	e.emitIndent()
	node.Type.Accept(e)
	e.emit([]byte(" "))
	e.emit([]byte(node.Name))
	e.emit([]byte(";\n"))
}

func (e *CppCodeEmitter) VisitMethod(node *CppMethod) {
	e.emitIndent()
	if node.ReturnType != nil {
		node.ReturnType.Accept(e)
		e.emit([]byte(" "))
	}
	e.emit([]byte(node.Name))
	e.emit([]byte("("))

	for i, param := range node.Params {
		if i > 0 {
			e.emit([]byte(", "))
		}
		param.Type.Accept(e)
		e.emit([]byte(" "))
		e.emit([]byte(param.Name))
	}

	e.emit([]byte(")"))
	if node.IsConst {
		e.emit([]byte(" const"))
	}
	e.emit([]byte(" { "))

	// Emit body
	for _, stmt := range node.Body {
		switch s := stmt.(type) {
		case *CppReturn:
			e.emit([]byte("return "))
			if ident, ok := s.Expr.(*CppIdent); ok {
				e.emit([]byte(ident.Name))
			}
			e.emit([]byte(";"))
		case *CppAssign:
			if lhs, ok := s.LHS.(*CppIdent); ok {
				e.emit([]byte(lhs.Name))
			}
			e.emit([]byte(" = "))
			if rhs, ok := s.RHS.(*CppIdent); ok {
				e.emit([]byte(rhs.Name))
			}
			e.emit([]byte(";"))
		}
	}

	e.emit([]byte(" }\n"))
}

func (e *CppCodeEmitter) VisitType(node *CppType) {
	if node.IsConst {
		e.emit([]byte("const "))
	}

	if node.Template != nil {
		e.emit([]byte(node.Base))
		e.emit([]byte("<"))
		for i, arg := range node.Template.Args {
			if i > 0 {
				e.emit([]byte(", "))
			}
			arg.Accept(e)
		}
		e.emit([]byte(">"))
	} else {
		e.emit([]byte(node.Base))
	}

	if node.IsRef {
		e.emit([]byte("&"))
	}
	if node.IsPointer {
		e.emit([]byte("*"))
	}
}

func (e *CppCodeEmitter) GetOutput() []byte {
	return e.output
}

func main() {
	goSource := `
package main

type Person struct {
ID    int64
Name  string
Email *string
Tags  []string
Meta  map[string]int
}

type Address struct {
Street  string
City    string
ZipCode *int
}
`

	transformer := NewGoToCppTransformer()
	if err := transformer.ParseGoSource(goSource); err != nil {
		panic(err)
	}

	cppAST := transformer.GetAST()

	emitter := &CppCodeEmitter{}
	cppAST.Accept(emitter)

	print(string(emitter.GetOutput()))
}
