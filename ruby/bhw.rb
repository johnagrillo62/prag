require "yaml"

module Bhw
  Path = File.dirname(__FILE__)
  YamlPath = File.join(File.dirname(__FILE__), "config.yaml")

  def self.load_yaml(file)
    YAML.unsafe_load(File.read(File.join(Path, file + ".yaml")))
  end
end

require "extend"
require "types"
require "mdb"
require "class"
require 'csharp'
require 'cpp'
require 'cpp26'
require 'go'
require 'fsharp'
require 'rust'
require 'ruby'
require 'python'
require 'java'
require 'csv'
require 'date'

module Bhw
Langs = 
  [#Go,
   #Rust,
   #FSharp,
   #CSharp,
   Cpp,
   Cpp26,
   #Ruby,
   #Java,
   #Python
  ]

def self.create_classes(class_def)
  out = []
  Langs.each do |mod|
    puts mod
    out << mod.write(class_def)
  end
  out
end




end

Writers = YAML.unsafe_load(File.read(Bhw::YamlPath))




