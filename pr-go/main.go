package main

import (
	"encoding/json"
	"flag"
	"fmt"
	"go/ast"
	"go/parser"
	"go/token"
	"io/ioutil"
	"os"
)

// Convert Go type to prag JSON type representation
func goTypeToPragJSON(expr ast.Expr) map[string]interface{} {
	switch t := expr.(type) {
	case *ast.Ident:
		return map[string]interface{}{
			"kind": "primitive",
			"name": t.Name,
		}
	case *ast.StructType:
		// Handle nested struct (always anonymous in Go)
		fields := []map[string]interface{}{}
		if t.Fields != nil {
			for _, field := range t.Fields.List {
				fieldName := ""
				if len(field.Names) > 0 {
					fieldName = field.Names[0].Name
				}
				if fieldName != "" {
					fields = append(fields, map[string]interface{}{
						"name": fieldName,
						"type": goTypeToPragJSON(field.Type),
					})
				}
			}
		}
		return map[string]interface{}{
			"kind":       "struct",
			"anonymous": true,
			"fields":    fields,
		}
	case *ast.ArrayType:
		return map[string]interface{}{
			"kind": "generic",
			"name": "Vec",
			"args": []map[string]interface{}{
				goTypeToPragJSON(t.Elt),
			},
		}
	case *ast.MapType:
		return map[string]interface{}{
			"kind": "generic",
			"name": "Map",
			"args": []map[string]interface{}{
				goTypeToPragJSON(t.Key),
				goTypeToPragJSON(t.Value),
			},
		}
	case *ast.StarExpr:
		return map[string]interface{}{
			"kind": "generic",
			"name": "Option",
			"args": []map[string]interface{}{
				goTypeToPragJSON(t.X),
			},
		}
	case *ast.SelectorExpr:
		if ident, ok := t.X.(*ast.Ident); ok {
			return map[string]interface{}{
				"kind": "primitive",
				"name": ident.Name + "." + t.Sel.Name,
			}
		}
		return map[string]interface{}{
			"kind": "primitive",
			"name": "unknown",
		}
	default:
		return map[string]interface{}{
			"kind": "primitive",
			"name": "unknown",
		}
	}
}

// Generate prag JSON struct from Go struct AST
func generatePragJSONStruct(name string, fields *ast.FieldList) map[string]interface{} {
	pragFields := []map[string]interface{}{}
	if fields != nil {
		for _, field := range fields.List {
			fieldName := ""
			if len(field.Names) > 0 {
				fieldName = field.Names[0].Name
			} else if ident, ok := field.Type.(*ast.Ident); ok {
				fieldName = ident.Name
			}
			if fieldName != "" {
				fieldType := goTypeToPragJSON(field.Type)
				pragFields = append(pragFields, map[string]interface{}{
					"name": fieldName,
					"type": fieldType,
				})
			}
		}
	}
	return map[string]interface{}{
		"type":   "Struct",
		"name":   name,
		"fields": pragFields,
	}
}

// Parse Go file and extract structs
func parseGoFile(source string) []map[string]interface{} {
	fset := token.NewFileSet()
	f, err := parser.ParseFile(fset, "", source, 0)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Parse error: %v\n", err)
		return []map[string]interface{}{}
	}

	var structs []map[string]interface{}

	for _, decl := range f.Decls {
		if genDecl, ok := decl.(*ast.GenDecl); ok {
			for _, spec := range genDecl.Specs {
				if typeSpec, ok := spec.(*ast.TypeSpec); ok {
					if structType, ok := typeSpec.Type.(*ast.StructType); ok {
						pragStruct := generatePragJSONStruct(typeSpec.Name.Name, structType.Fields)
						structs = append(structs, pragStruct)
					}
				}
			}
		}
	}

	return structs
}

func main() {
	inputFile := flag.String("input", "", "Input Go file (default: stdin)")
	flag.Parse()

	var source string
	var err error

	if *inputFile != "" {
		data, err := ioutil.ReadFile(*inputFile)
		if err != nil {
			fmt.Fprintf(os.Stderr, "Error reading file: %v\n", err)
			os.Exit(1)
		}
		source = string(data)
	} else {
		data, err := ioutil.ReadAll(os.Stdin)
		if err != nil {
			fmt.Fprintf(os.Stderr, "Error reading stdin: %v\n", err)
			os.Exit(1)
		}
		source = string(data)
	}

	structs := parseGoFile(source)

	pragAST := map[string]interface{}{
		"type":           "Module",
		"sourceLanguage": "go",
		"items":          structs,
	}

	jsonBytes, err := json.MarshalIndent(pragAST, "", "  ")
	if err != nil {
		fmt.Fprintf(os.Stderr, "Error marshaling JSON: %v\n", err)
		os.Exit(1)
	}

	fmt.Println(string(jsonBytes))
}