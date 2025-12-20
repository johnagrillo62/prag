# The Evolution of astrei: A 20-Year Journey

## Timeline: From Ruby to Universal Schema Transpiler

### ~2005: Ruby YAML Codegen
**The Beginning**

```ruby
# Pure Ruby stdlib - no gems!
require 'yaml'
require 'erb'

data = YAML.load_file('types.yaml')
template = ERB.new(File.read('template.erb'))
output = template.result(binding)
```

**Pros:**
- ✅ Simple, elegant
- ✅ No dependencies beyond Ruby
- ✅ Easy to understand

**Cons:**
- ❌ Stringly-typed (no validation)
- ❌ YAML maintenance nightmare
- ❌ Template hell for multiple languages
- ❌ Ruby-only codegen

---

### JUne 2025: MetaFront (libclang)
**The "Proper" Solution**

Leveraged libclang for full C++ AST parsing and reflection metadata generation.

**Pros:**
- ✅ Real C++ parsing
- ✅ Tuple-based metadata
- ✅ Function reflection
- ✅ Cross-language generation

**Cons:**
- ❌ 500MB+ LLVM dependency
- ❌ Not portable
- ❌ Complex build process
- ❌ Heavy installation requirements

**The Problem:**
```bash
apt install libclang-dev  # 100MB+
# or
# Download LLVM (500MB+)
```

---

### July 2025: D Language Compile-Time Reflection
**The Elegant Approach**

D has built-in compile-time reflection - seemed perfect!

```d
import std.traits;

auto generateCode(T)() {
    foreach(member; FieldNameTuple!T) {
        // Generate code at compile time
    }
}
```

**Pros:**
- ✅ Elegant metaprogramming
- ✅ Built-in reflection
- ✅ No external dependencies
- ✅ Type-safe

**Cons:**
- ❌ **Can't parse external D files**
- ❌ Reflection only works on code being compiled
- ❌ Can't distribute D compiler
- ❌ Requires target code to be compiled

**Fatal Flaw:** Compile-time reflection can't read external files.

---

### 2025: Rust syn Crate
**The Macro Approach**

```rust
use syn::{parse_file, Item};
use quote::quote;

let ast = parse_file(&source)?;
```

**Pros:**
- ✅ Powerful AST parsing
- ✅ Great for procedural macros

**Cons:**
- ❌ **Designed for macros, not general parsing**
- ❌ Heavyweight dependencies:
  ```bash
  cargo add syn
  cargo add quote  
  cargo add proc-macro2
  ```
- ❌ Compile time overhead
- ❌ Rust-only (can't parse C++, Go, etc.)

**Fatal Flaw:** Tied to Rust compilation pipeline, language-specific.

---

### 2025: Go go/ast Package
**The Standard Library Approach**

```go
import (
    "go/ast"
    "go/parser"
    "go/token"
)

fset := token.NewFileSet()
node, err := parser.ParseFile(fset, filename, nil, 0)
```

**Pros:**
- ✅ Standard library
- ✅ Can parse external `.go` files
- ✅ Well-documented

**Cons:**
- ❌ **Go-only** (can't parse Rust, C++, Proto, etc.)
- ❌ Needs Go toolchain installed
- ❌ Verbose for code generation

**Fatal Flaw:** Language-specific parser.

---

### 2024: Python ast Module
**The Runtime Approach**

```python
import ast

tree = ast.parse(source_code)
for node in ast.walk(tree):
    # Process AST
```

**Pros:**
- ✅ Can parse external `.py` files
- ✅ Standard library
- ✅ Easy string manipulation

**Cons:**
- ❌ **Python-only** (can't parse other languages)
- ❌ pip dependencies for anything beyond basics
- ❌ virtualenv/venv management
- ❌ Runtime-only reflection

**Fatal Flaw:** Language-specific, dependency hell.

---

## The Universal Problem

Every language's AST tool has the **same fundamental flaw**:

| Language | Can Parse External Files? | Universal? | Dependencies |
|----------|--------------------------|------------|--------------|
| D        | ❌ No                    | ❌ No      | D compiler   |
| Rust syn | ✅ Yes                   | ❌ No      | cargo + crates |
| Go       | ✅ Yes                   | ❌ No      | Go toolchain |
| Python   | ✅ Yes                   | ❌ No      | pip + venv   |

**They're all language-specific. None could be universal.**

---

## 2025: astrei - Simple Text Parsing
**The Right Solution**

### Core Insight
**Just parse text patterns. Language-agnostic.**

```
ASCII text (any format)
    ↓
Simple text parser (no dependencies)
    ↓
Universal AST (common representation)
    ↓
Generate any output language
```

### Architecture

```
┌─────────────┐                    ┌──────────────┐
│  Protocol   │───┐            ┌───│     C++      │
│   Buffers   │   │            │   │    headers   │
├─────────────┤   │            │   ├──────────────┤
│  Cap'n      │   │            │   │      Go      │
│   Proto     │   │            │   ├──────────────┤
├─────────────┤   │    AST     │   │     Java     │
│ FlatBuffers │───┼────────────┼───├──────────────┤
├─────────────┤   │            │   │    Proto     │
│   GraphQL   │   │            │   ├──────────────┤
│     C++     │   │            │   │    Python    │
├─────────────┤   │            │   ├──────────────┤
│     Go      │   │            │   │     Rust     │
├─────────────┤   │            │   ├──────────────┤
│    Rust     │   │            │   │     Zig      │
├─────────────┤   │            │   └──────────────┘
│     Avro    │   │
├─────────────┤   │
│   Thrift    │   │
├─────────────┤   │
│  MDB/Access │   │
├─────────────┤   │
│ JSON Schema │───┘
└─────────────┘
   12 Parsers        Unified AST       7 Generators
```

### What astrei Provides

**Input Formats (12):**
- Protocol Buffers (.proto)
- Cap'n Proto (.capnp)
- FlatBuffers (.fbs)
- GraphQL (.gql, .graphql)
- C++ headers (.h, .hpp, .cpp)
- Go (.go)
- Rust (.rs)
- Python (.py)
- Apache Avro (.avsc)
- Apache Thrift (.thrift)
- MS Access (.mdb via mdb-tools)
- JSON Schema (.json)

**Output Languages (7):**
- C++ (headers with STL types)
- Go (with JSON tags)
- Java (public fields, proper imports)
- Protocol Buffers (with field numbering)
- Python (dataclasses with type hints)
- Rust (with derives)
- Zig (with ArrayList handling)

**Total Conversion Paths:** 12 × 7 = **84 conversions**

### Build & Run

```bash
# No dependencies, no package managers, no installation
g++ -std=c++17 *.cpp -o astrei

# Use it
./astrei schema.proto output.rs    # Proto → Rust
./astrei models.go models.java     # Go → Java  
./astrei api.capnp api.py          # Cap'n Proto → Python
./astrei database.mdb structs.zig  # Access → Zig
```

### Why It Works

**No Dependencies:**
- ❌ No package managers (pip, cargo, npm, gem)
- ❌ No language runtimes
- ❌ No compilation toolchains
- ❌ No virtual environments
- ✅ Just compile and run

**Universal:**
- Parse ANY schema format
- Generate ANY target language
- Add new formats easily (just write a parser)
- Add new targets easily (just write a generator)

**Simple:**
- ~3500 lines total across all parsers
- Text pattern matching
- No AST libraries
- No heavyweight dependencies

**Fast:**
- Instant compilation
- Zero runtime overhead
- Batch process thousands of files

**Deterministic:**
- Same input → Same output (always)
- No AI hallucinations
- No "creative interpretations"
- Perfect for CI/CD

---

## Key Lessons

### 1. Complex Solutions Often Wrong
- libclang: Too heavy
- Language AST libs: Too specific
- Compile-time reflection: Too limited

### 2. Simple Text Parsing Wins
- Language-agnostic
- No dependencies
- Easy to understand
- Easy to extend

### 3. Universal > Specialized
- One tool for all conversions
- Don't need language-specific tooling
- Portable everywhere

### 4. Installation Is A Feature
```bash
# Ruby: gem install ... (dependency hell)
# MetaFront: apt install libclang-dev (100MB+)
# Rust: cargo add ... (compile time)
# Go: go get ... (need toolchain)
# Python: pip install ... (venv hell)

# astrei: g++ *.cpp -o astrei ✅
```

---

## Real-World Use Cases

1. **Proto → Cap'n Proto** - Migrate from Google to CloudFlare
2. **GraphQL → Proto** - Convert schemas to gRPC
3. **MDB → FlatBuffers** - Modernize legacy Access databases
4. **C++ → Go** - Auto-generate Go bindings
5. **FlatBuffers → Rust** - Game engine schemas
6. **Any → Proto** - Universal migration path

---

## The 20-Year Arc

```
2015: Ruby YAML
  ↓
2025: MetaFront/libclang  
  ↓
2025: D reflection → Rust syn → Go ast → Python ast
  ↓
2025: Text parsing (astrei)
```

**Had to try everything else to find the simple solution.**

**Complexity → Simplicity → Success** 🎯

---

## Stats

- **Years of development:** 20
- **Failed approaches:** 6
- **Languages attempted:** D, Rust, Go, Python, Ruby, C++
- **Lines of code (astrei):** ~3500
- **Dependencies:** 0
- **Input formats:** 12
- **Output languages:** 7
- **Conversion paths:** 84
- **Installation steps:** 1 (compile)

---

## Conclusion

**astrei is what happens when you:**
- Try the complex solutions first
- Realize they all have the same flaw
- Step back and think fundamentally
- Choose simple over clever
- Build universal over specialized

**20 years. 7 attempts. 1 solution.**

#   p r a g  
 #   p r a g  
 