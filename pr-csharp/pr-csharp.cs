using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.CSharp.Syntax;
using Newtonsoft.Json.Linq;

namespace CSharpAstParser
{
    class Program
    {
        static void Main(string[] args)
        {
            if (args.Length == 0 || args[0] == "--help" || args[0] == "-h")
            {
                PrintHelp();
                return;
            }

            if (args[0] == "--usage")
            {
                PrintUsage();
                return;
            }

            string? inputFile = null;
            string? outputFile = null;

            // First argument is input file
            if (args[0] != null && !args[0].StartsWith("-"))
            {
                inputFile = args[0];
            }

            // Parse remaining arguments
            for (int i = 1; i < args.Length; i++)
            {
                switch (args[i])
                {
                    case "-o":
                    case "--output":
                        if (i + 1 < args.Length)
                        {
                            outputFile = args[++i];
                        }
                        else
                        {
                            Console.Error.WriteLine("Error: --output requires a filename argument");
                            PrintUsage();
                            Environment.Exit(1);
                        }
                        break;

                    case "-h":
                    case "--help":
                        PrintHelp();
                        return;

                    default:
                        Console.Error.WriteLine($"Error: Unknown argument '{args[i]}'");
                        PrintUsage();
                        Environment.Exit(1);
                        break;
                }
            }

            // If no input file specified, error
            if (string.IsNullOrEmpty(inputFile))
            {
                Console.Error.WriteLine("Error: No input file specified");
                PrintUsage();
                Environment.Exit(1);
            }

            try
            {
                if (!File.Exists(inputFile))
                {
                    Console.Error.WriteLine($"Error: Input file '{inputFile}' not found");
                    Environment.Exit(1);
                }

                var structs = ParseStructsFromFile(inputFile);
                var items = new List<JObject>();

                foreach (var structDecl in structs)
                {
                    items.Add(GeneratePragJsonStruct(structDecl));
                }

                var pragAst = new JObject
                {
                    { "type", "Module" },
                    { "items", JArray.FromObject(items) }
                };

                string output = pragAst.ToString(Newtonsoft.Json.Formatting.Indented);

                // Write to file or stdout
                if (!string.IsNullOrEmpty(outputFile))
                {
                    File.WriteAllText(outputFile, output);
                    Console.Error.WriteLine($"Output written to '{outputFile}'");
                }
                else
                {
                    Console.WriteLine(output);
                }
            }
            catch (Exception ex)
            {
                Console.Error.WriteLine($"Error: {ex.Message}");
                Environment.Exit(1);
            }
        }

        static void PrintUsage()
        {
            Console.Error.WriteLine("Usage: program INPUT_FILE [OPTIONS]");
            Console.Error.WriteLine("\nOptions:");
            Console.Error.WriteLine("  -o, --output FILE   Write JSON output to FILE (default: stdout)");
            Console.Error.WriteLine("  -h, --help          Show this help message");
            Console.Error.WriteLine("  --usage             Show this usage message");
        }

        static void PrintHelp()
        {
            Console.WriteLine("C# Struct to PragJSON Parser");
            Console.WriteLine("\nDescription:");
            Console.WriteLine("  Parses C# struct declarations and converts them to PragJSON AST format.");
            Console.WriteLine("\nUsage:");
            Console.WriteLine("  program INPUT_FILE [OPTIONS]");
            Console.WriteLine("\nArguments:");
            Console.WriteLine("  INPUT_FILE          C# source file containing struct declarations");
            Console.WriteLine("\nOptions:");
            Console.WriteLine("  -o, --output FILE   Output file for JSON AST (default: stdout)");
            Console.WriteLine("  -h, --help          Show this help message and exit");
            Console.WriteLine("  --usage             Show brief usage information and exit");
            Console.WriteLine("\nExamples:");
            Console.WriteLine("  program examples.cs");
            Console.WriteLine("  program examples.cs -o output.json");
            Console.WriteLine("  program --help");
        }

        static JObject CSharpTypeToPragJson(TypeSyntax typeSyntax)
        {
            if (typeSyntax is IdentifierNameSyntax id)
                return new JObject { { "kind", "primitive" }, { "name", id.Identifier.ValueText } };
            
            if (typeSyntax is GenericNameSyntax gen)
            {
                var args = gen.TypeArgumentList.Arguments.Select(CSharpTypeToPragJson).ToList();
                string name = gen.Identifier.ValueText switch
                {
                    "List" => "Vec",
                    "Dictionary" => "Map",
                    "HashSet" => "Set",
                    _ => gen.Identifier.ValueText
                };
                return new JObject { { "kind", "generic" }, { "name", name }, { "args", JArray.FromObject(args) } };
            }
            
            if (typeSyntax is NullableTypeSyntax nul)
            {
                var inner = CSharpTypeToPragJson(nul.ElementType);
                return new JObject { { "kind", "generic" }, { "name", "Option" }, { "args", JArray.FromObject(new[] { inner }) } };
            }
            
            return new JObject { { "kind", "primitive" }, { "name", typeSyntax.ToString() } };
        }

        static JObject GeneratePragJsonStruct(StructDeclarationSyntax structDecl)
        {
            var fields = new List<JObject>();
            
            foreach (var member in structDecl.Members)
            {
                if (member is PropertyDeclarationSyntax prop)
                {
                    fields.Add(new JObject { { "name", prop.Identifier.ValueText }, { "type", CSharpTypeToPragJson(prop.Type) } });
                }
                else if (member is FieldDeclarationSyntax field)
                {
                    foreach (var var in field.Declaration.Variables)
                    {
                        fields.Add(new JObject { { "name", var.Identifier.ValueText }, { "type", CSharpTypeToPragJson(field.Declaration.Type) } });
                    }
                }
            }
            
            return new JObject { { "type", "Struct" }, { "name", structDecl.Identifier.ValueText }, { "fields", JArray.FromObject(fields) } };
        }

        static List<StructDeclarationSyntax> ParseStructsFromFile(string path)
        {
            var src = File.ReadAllText(path);
            var tree = CSharpSyntaxTree.ParseText(src);
            var root = (CompilationUnitSyntax)tree.GetRoot();
            return root.DescendantNodes().OfType<StructDeclarationSyntax>().ToList();
        }
    }
}
