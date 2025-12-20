require_relative "writer"


module Python


  @types = Bhw::load_yaml("python")

  def self.to_type(type)
    clss = type.class.name.split("::").last
    type_def = @types["python_types"][clss]["type"]
    return type_def
  end
    

    
  def self.to_convert(type)
    clss = type.class.name.split("::").last
    type_def = @types["ruby_types"][clss]

    # Return the class type based on the class and type
    if type_def
      return type_def['convert']
    end
    puts "Unknown type: #{type}"
    nil
  end

  def self.write(class_def)
    members = class_def.members
    writer = Writer.new(Writers["python"])
    
    class_name = writer.to_class_name(class_def.class_name)
    if class_name[0].match?(/\d/)
      class_name = "T" + class_name
    end
    
    ext_src = writer.source_file_ext
    src = writer.to_file_name(class_def.class_name) + "." + ext_src
    
    # does the table have a unique id
    
    indexed = class_def.uniques.select{|n| class_name.downcase == n.downcase}[0]
    if indexed
      indexed = writer.to_member_name(indexed) + "_"
    end
    

    out = [
      class_def.schema.split("\n").reject{|l| l.start_with?("---") }.map{|l| "# #{l.squish}"} * "\n",
      "",
      "from src.python.tm import meta",
      "import datetime",
      "",
      "class #{class_name}(metaclass=meta.MetaDataClass):",
      "    FIELDS = [",
      members.map { |m|
        [
          "        { \"type\": ",
          to_type(m.type),
          ", \"member\": \"",
          writer.to_member_name(m.member_name),
          "\", \"column\": \"",
          m.column_name,
          "\"},"
        ].join("")
      }.join("\n") + "\n    ]\n"
    ]
    
    ["python", [[src, out.compact * "\n"]]]
  end
end


