

module Java

  @types = Bhw::load_yaml("java")
  def self.to_type(type)
    clss = type.class.name.split("::").last
    type_def = @types["java_types"][clss]

    # Return the class type based on the class and type
    if type_def
      case type
      when Types::Integer
        return type_def['types'][type.bits]['wrapper']['signed']['type']
      when Types::Float
        return type_def['types'][type.type.to_s]['wrapper']['type']
      when Types::Boolean
        return type_def['types']['wrapper']['type']
      else
        return type_def['type']
      end
    end
  end

  
  def self.write(class_def)
    writer = Writer.new(Writers["java"])    
    
    ext_src = writer.source_file_ext
    
    class_name = writer.to_class_name(class_def.class_name)
    src_name = "#{writer.to_file_name(class_def.class_name)}.#{ext_src}"
    src_name = writer.to_file_name(class_def.class_name) + "." + ext_src

    indexed = class_def.uniques.any?

    key = class_def.members.select{|m| m.unique}
    if (key.size > 0)
      key = writer.to_reader_name(key[0].column_name)
    end

    src = [
      "package hytek.#{class_def.package};",
      "",
      "import com.opencsv.bean.CsvBindByName;",
      "",
      "public class #{class_name} {",
      "",
      "    public #{class_name} () {;}",
      "",
      "    public #{class_name}(" + class_def.members.map do |m|
        to_type(m.type) + " " + writer.parameter_str(m.member_name)
      end * ", " + " ) {",
      class_def.members.map do |m|
        "        this." + writer.to_member_name(m.member_name) + " = " + writer.parameter_str(m.member_name) + ";"
      end.join("\n"),
      "    }",
      "",
      
      class_def.members.map do |m|
        m_name = m.member_name
        m_type = to_type(m.type)
        mname = writer.to_member_name(m_name)
        pname = writer.parameter_str(m_name)
        wstr = writer.to_writer_name(m_name)
        rstr = writer.to_reader_name(m_name)
        [
          "    public #{m_type} #{rstr}() {",
          "        return "  + writer.to_member_name(m_name) + ";",
          "    }",
        ].join("\n") + "\n\n"
      end.join,
      
      #"  static constexpr char query[] = \"select " + class_def.getSelectFields + " from " + class_def.table_name + "\";",
      class_def.members.map do |m|
        m_type = to_type(m.type)
        mname = writer.to_member_name(m.member_name)
        
        "    @CsvBindByName(column = \"#{m.column_name}\")\n    private #{m_type} #{mname};\n"
      end * "\n",
      "};\n",
    ]


    ["java", [[src_name, src]]]    
  end
end

