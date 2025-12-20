#!/usr/bin/env ruby
# spec_generator.rb

require 'yaml'
require 'fileutils'

class CppGenerator
  def initialize(spec)
    @spec = spec
  end
  
  def generate
    code = []
    code << "// AUTO-GENERATED - DO NOT EDIT"
    code << "#pragma once"
    code << "#include <string>"
    code << "#include <map>"
    code << ""
    code << "namespace codegen {"
    code << ""
    
    # Generate enum
    code << "enum class Language {"
    @spec['languages'].each do |name, _|
      code << "    #{to_enum_name(name)},"
    end
    code << "};"
    code << ""
    
    # Generate structs
    code << "struct PrimitiveType {"
    code << "    std::string type_name;"
    code << "    std::string default_value;"
    code << "};"
    code << ""
    
    code << "struct LanguageInfo {"
    code << "    std::string file_ext;"
    code << "    std::string comment_style;"
    code << "    std::map<std::string, PrimitiveType> primitives;"
    code << "    std::map<std::string, PrimitiveType> generics;"
    code << "};"
    code << ""
    
    # Generate registry
    code << "inline const std::map<Language, LanguageInfo> LANGUAGE_REGISTRY = {"
    @spec['languages'].each do |name, spec|
      code << "    {Language::#{to_enum_name(name)}, {"
      code << "        .file_ext = \"#{spec['config']['file_extension']}\","
      code << "        .comment_style = \"#{spec['config']['comment_style']}\","
      code << "        .primitives = {"
      
      spec['primitives'].each do |prim_name, prim_spec|
        code << "            {\"#{prim_name}\", {\"#{prim_spec[0]}\", \"#{prim_spec[1]}\"}},"
      end
      code << "        },"

      
      code << "        .standard = {"
      spec['standard'].each do |name, (type,default)|
        code << "            {\"#{name}\", {\"#{type}\", \"#{default}\"}},"
      end
      code << "        },"
      
      code << "        .generics = {"
      
      spec['generics'].each do |gen_name, gen_spec|
        code << "            {\"#{gen_name}\", {\"#{gen_spec[0]}\", \"#{gen_spec[1]}\"}},"
      end
      
      code << "        }"
      code << "    }},"
    end
    code << "};"
    code << ""
    code << "} // namespace codegen"
    
    code.join("\n")
  end
  
  private
  
  def to_enum_name(name)
    name.split('_').map(&:capitalize).join
  end
end

class RustGenerator
  def initialize(spec)
    @spec = spec
  end
  
  def generate
    code = []
    code << "// AUTO-GENERATED - DO NOT EDIT"
    code << "use std::collections::HashMap;"
    code << "use once_cell::sync::Lazy;"
    code << ""
    
    # Enum
    code << "#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]"
    code << "pub enum Language {"
    @spec['languages'].each do |name, _|
      code << "    #{to_enum_name(name)},"
    end
    code << "}"
    code << ""
    
    # Structs
    code << "pub struct PrimitiveType {"
    code << "    pub type_name: &'static str,"
    code << "    pub default_value: &'static str,"
    code << "}"
    code << ""
    
    code << "pub struct LanguageInfo {"
    code << "    pub file_ext: &'static str,"
    code << "    pub comment_style: &'static str,"
    code << "    pub primitives: HashMap<&'static str, PrimitiveType>,"
    code << "    pub generics: HashMap<&'static str, PrimitiveType>,"
    code << "}"
    code << ""
    
    # Registry
    code << "pub static LANGUAGE_REGISTRY: Lazy<HashMap<Language, LanguageInfo>> = Lazy::new(|| {"
    code << "    let mut map = HashMap::new();"
    code << ""
    
    @spec['languages'].each do |name, spec|
      code << "    map.insert(Language::#{to_enum_name(name)}, LanguageInfo {"
      code << "        file_ext: \"#{spec['config']['file_extension']}\","
      code << "        comment_style: \"#{spec['config']['comment_style']}\","
      code << "        primitives: ["
      
      spec['primitives'].each do |prim_name, prim_spec|
        code << "            (\"#{prim_name}\", PrimitiveType {"
        code << "                type_name: \"#{prim_spec[0]}\","
        code << "                default_value: \"#{prim_spec[1]}\","
        code << "            }),"
      end
      
      code << "        ].into_iter().collect(),"
      code << "        generics: ["
      
      spec['generics'].each do |gen_name, gen_spec|
        code << "            (\"#{gen_name}\", PrimitiveType {"
        code << "                type_name: \"#{gen_spec[0]}\","
        code << "                default_value: \"#{gen_spec[1]}\","
        code << "            }),"
      end
      
      code << "        ].into_iter().collect(),"
      code << "    });"
      code << ""
    end
    
    code << "    map"
    code << "});"
    
    code.join("\n")
  end
  
  private
  
  def to_enum_name(name)
    name.split('_').map(&:capitalize).join
  end
end

# Main
if __FILE__ == $0
  spec = YAML.load_file('lang.yaml')
  
  case ARGV[0]
  when 'cpp'
    code = CppGenerator.new(spec).generate
    File.write(ARGV[1] || 'generated_languages.hpp', code)
    puts "✓ Generated C++"
    
  when 'rust'
    code = RustGenerator.new(spec).generate
    File.write(ARGV[1] || 'generated_languages.rs', code)
    puts "✓ Generated Rust"
    
  when 'all'
    File.write('languages.h', CppGenerator.new(spec).generate)
    File.write('languages.rs', RustGenerator.new(spec).generate)
    puts "✓ Generated all languages"
    
  else
    puts "Usage: #{$0} [cpp|rust|all] [output_file]"
    exit 1
  end
end
