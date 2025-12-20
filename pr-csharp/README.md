# C# Struct to PragJSON Parser

A command-line tool that parses C# struct declarations and converts them to PragJSON AST format.

## Installation

### 1. Install .NET SDK 8.0

#### Windows

Download from: https://dotnet.microsoft.com/download/dotnet/8.0

Run the installer and follow the prompts.

#### macOS

Using Homebrew:
```bash
brew install dotnet
```

Or download from: https://dotnet.microsoft.com/download/dotnet/8.0

#### Linux (Ubuntu/Debian)

```bash
wget https://dot.net/v1/dotnet-install.sh -O dotnet-install.sh
chmod +x dotnet-install.sh
./dotnet-install.sh --version 8.0
```

Or using apt:
```bash
sudo apt-get install dotnet-sdk-8.0
```

### 2. Verify installation

```bash
dotnet --version
```

Should output version 8.0 or higher.

### 3. Install NuGet packages

Navigate to your project directory and run:

```bash
dotnet restore
```

This will automatically install:
- **Microsoft.CodeAnalysis.CSharp 5.0.0** (Roslyn)
- **Newtonsoft.Json 13.0.4** (JSON)

These are already listed in `CSharpParser.csproj`.

## Quick Start

### Run the parser

```bash
dotnet run -- examples.cs
```

Or with output file:

```bash
dotnet run -- examples.cs -o output.json
```

## Project Files

- `CSharpParser.csproj` - Project configuration (already set up)
- `pr-csharp.cs` - Main parser implementation
- `examples.cs` - Example C# structs to parse

## Usage

### Basic usage (output to console)

```bash
dotnet run -- examples.cs
```

### Save output to file

```bash
dotnet run -- examples.cs -o output.json
```

### Show help

```bash
dotnet run -- --help
```

### Show usage

```bash
dotnet run -- --usage
```

## Command-line Options

```
Usage: program INPUT_FILE [OPTIONS]

Arguments:
  INPUT_FILE          C# source file containing struct declarations

Options:
  -o, --output FILE   Write JSON output to FILE (default: stdout)
  -h, --help          Show this help message
  --usage             Show brief usage information
```

## Examples

Parse structs and print JSON to console:
```bash
dotnet run -- examples.cs
```

Parse structs and save to file:
```bash
dotnet run -- examples.cs -o output.json
```

Show help:
```bash
dotnet run -- --help
```

## How it works

1. Reads a C# source file using Roslyn parser
2. Extracts all struct declarations
3. Converts struct fields and properties to PragJSON format
4. Outputs a JSON AST with the following structure:

```json
{
  "type": "Module",
  "items": [
    {
      "type": "Struct",
      "name": "StructName",
      "fields": [
        {
          "name": "fieldName",
          "type": {
            "kind": "primitive",
            "name": "int"
          }
        }
      ]
    }
  ]
}
```

## Type Mappings

Generic types are mapped to PragJSON equivalents:

- `List<T>` → `Vec`
- `Dictionary<K,V>` → `Map`
- `HashSet<T>` → `Set`
- `T?` → `Option<T>`

Primitive types (int, string, bool, etc.) remain as-is.

## Dependencies

The project uses:

- **Microsoft.CodeAnalysis.CSharp 5.0.0** - C# syntax tree parsing (Roslyn)
- **Newtonsoft.Json 13.0.4** - JSON serialization

These are already configured in `CSharpParser.csproj`.

## Building for Release

Create a standalone executable:

```bash
dotnet publish -c Release -o ./publish
```

Then run directly without `dotnet run`:

```bash
./publish/CSharpParser examples.cs -o output.json
```

(On Windows: `.\publish\CSharpParser.exe examples.cs -o output.json`)

## Troubleshooting

### "dotnet: command not found"
Install .NET SDK from https://dotnet.microsoft.com/download

### "Could not find project or directory"
Make sure you're in the project directory containing `CSharpParser.csproj`

### "Could not find file examples.cs"
The input file must exist. Check the filename and path.

### Build errors about packages
Run: `dotnet restore`

## Project Structure

```
.
├── CSharpParser.csproj          # Project configuration
├── pr-csharp.cs                 # Main parser code
├── examples.cs                  # Example input file
└── README.md                    # This file
```

## License

MIT