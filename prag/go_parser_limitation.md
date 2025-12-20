# Go Variant Interface Limitation

- **C# abstract records** - Same issue with inheritance
- **F# discriminated unions** - Was fixed by adding Oneof support to walker

## Possible Solutions

1. **Extend AST** - Add metadata to StructRef indicating original Variant structure
2. **Name-based detection** - Recognize "Variant0" pattern and try to reconstruct
3. **Accept limitation** - Document that Go variants are one-way only

## Status

**Known architectural limitation** - would require AST changes to fix properly.
