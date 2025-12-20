# C++
#  classes
#
#  members
#    public private protected
#    getter setter
#
# type
#   optional
#
# namespaces

module Cpp26
  @types = Bhw::load_yaml("cpp26")

  def self.to_type(type)
    clss = type.class.name.split("::").last
    type_def = @types["cpp_types"][clss]["types"]

    # Return the class type based on the class and type
    if type_def
      case type
      when Types::Integer
        return type_def[type.bits]['unsigned']['required']['type']
      when Types::Float
        puts type.type
        return type_def[type.type.to_s]['required']['type']
      else
        return type_def['required']['type']

      end
    end
    puts "Unknown type: #{type}"
    nil
  end

  def self.to_convert(type, val)
    clss = type.class.name.split("::").last
    type_def = @types["cpp_types"][clss]["types"]

    convert =
      # Return the class type based on the class and type
      if type_def
        case type
        when Types::Integer
          type_def[type.bits]['signed']['required']['convert']
        when Types::Float
          type_def[type.type.to_s]['required']['convert']
        else
          type_def['required']['convert']
        end
      end
    if convert == nil
      val
    else
      "#{convert}(#{val})"
    end

  end

  def self.to_read(type)
    case type
    when Types::String
      "bhw::GetText"
    when Types::Boolean
      "bhw::GetBool"
    when Types::DateTime
      "bhw::GetTimePoint"
    when Types::Integer
      if (type.bits == 8)
        "bhw::GetChar"
      elsif (type.bits == 16)
        "bhw::GetShort"
      elsif (type.bits == 32)
        "bhw::GetInt"
      else
        "bhw::GetLong"
      end
    else
      ""
    end
  end

  def self.meta(class_name, clss, wrap=false)
    out = []

    out << "namespace meta"  << "{"
    out << "namespace hytek"
    out << "{"
    out << "namespace tm"
    out << "{"
    
    out << "namespace #{clss.class_name.camel_capitalize_case}"
    out << "{"
    out << "inline constexpr char tableName[] = \"#{class_name}\"; "
    out << ""
    out << "inline constexpr char query[] = R\"lit(select " + clss.getSelectFields + " from " + clss.table_name + ")lit\"; "
    out << ""
    
    out << "static const auto fields = std::make_tuple("
    out << clss.members.map {|m|
      
      attrs = "meta::Prop::Serializable | meta::Prop::Hashable"
      if m.unique == true
        attrs = "meta::Prop::PrimaryKey | " + attrs
      end

      mtype = self.to_type(m.type)


      ["        meta::Field<",
       "::hytek::tm::" + class_name,
       ", ",
       mtype,
       ", ",
       "&",
       "::hytek::tm::" + class_name,
       "::",
       m.member_name.camel_case,
       ", ",
       attrs,
       ", nullptr",
       ", nullptr",
       ">{",
       "\"" + mtype + "\", ",
       "\"" + m.member_name.camel_case + "\" ,{ ",
       "}}"
      ] * ""
    } * ",\n"

    out << ");"

    out << "}"

    out << "}"
    out << "}"
    out << "}"

    out << ""

    out << "namespace meta"
    out << "{"

    full = "::hytek::tm::#{clss.class_name.camel_capitalize_case}"
    puts full
    
    out << "template <> struct MetaTuple<#{full}>"
    out << "{"
    out << "   static inline const auto& fields = meta#{full}::fields;"
    out << "   static constexpr auto tableName = meta#{full}::tableName;"
    out << "   static constexpr auto query = meta#{full}::query;"
    out << "};"
    out << "} // namespace meta"
    

    
    # out << clss.members.map {|m|
    #   mtype = self.to_type(m.type)

    #   [
    #     "        {",

    #     "\"" + m.member_name.camel_case + "\", ",
    #     "\"" + m.member_name.camel_case + "\", ",
    #     "\"" + m.member_name.camel_case + "\"",
    #     "}",        
    #   ] * ""
      
    # } * ",\n"
    # out << "    };"
    
    out * "\n"

    
  end
  def self.write_read(members, dbase, clss, writer, package)
    out = []
    out << ""
    out << "// ReSharper disable CppInconsistentNaming"
    out << "namespace";
    out << "{"
    members.each_with_index.map do |m, i|
      field = m.member_name.camel_case +  "FieldNum"
      out << "constexpr uint16_t #{field} = #{i + 1};"

      if m.type.class == Types::String
        field = m.member_name.camel_case + "FieldLength"
        out << "constexpr int #{field} = #{m.type.len.to_s};";
      end

    end
    out << "} // namespace"
    out << ""
    out << "template<>"
    out << "auto bhw::ReadObj(const SQLHANDLE& stmt) -> #{package}::#{clss}"
    out << "{"
    members.each_with_index.map do |m, i|

      field = m.member_name.camel_case + "FieldNum"

      assign = "  auto #{m.member_name.camel_case} = " + to_read(m.type) + "(stmt, " + field
      if m.type.class == Types::String
        field = m.member_name.camel_case + "FieldLength"
        assign += ", " + field
      end
      out << assign + ");"
    end
    out << "  return #{package}::#{clss}::from({" + members.each_with_index.map do |m, i|
      "." + writer.parameter_str(m.member_name) + " = #{m.member_name.camel_case}"
    end * ", " + "});"
    out << "}"

    out << "template<>"
    out << "auto bhw::ReadUniquePtr(const SQLHANDLE& stmt) -> std::unique_ptr<#{package}::#{clss}>"
    out << "{"
    out << "  return std::make_unique<#{package}::#{clss}>(ReadObj<#{package}::#{clss}>(stmt));"
    out << "}"
    out * "\n"
  end

  def self.write(class_def)
    writer = Writer.new(Writers["cpp26"])

    members = class_def.members
    ext_hdr = writer.header_file_ext
    ext_src = writer.source_file_ext

    dir = class_def.package * "/"
    base_file_path = writer.to_file_name(class_def.class_name)
    hdr_non_const_file_path = File.join(dir, base_file_path + "." + ext_hdr)
    
    class_name_non_const = writer.to_class_name(class_def)
    meta_file_non_const_path = File.join(dir, base_file_path + ".meta")    

    package = class_def.package.map { |c| c.strip } * "::"
    indexed = if class_def.uniques.size == 0
                false
              else
                true
              end
    class_name = writer.to_class_name(class_def)
    key = class_def.members.select { |m| m.unique }
    if key.size > 0
      key = writer.to_reader_name(key[0].member_name)
    end

    meta_src = [
      self.meta(class_name_non_const, class_def),
    ] * "\n"

    hdr_non_const = [
      "#pragma once",
      "#include <chrono>",
      "#include <string>",
      
      "",
      "namespace #{package}",
      "{",
      "struct #{class_name} ",
      "{",
      members.each_with_index.map do |m, i|
        m_type = to_type(m.type)
        mname = m.member_name.camel_case
        [
          "    #{m_type} #{mname};"
        ] * "\n"
      end * "\n",
      "};\n",

      "} //namespace",
      "#include \"#{meta_file_non_const_path}\"\n",
    ]

    ["cpp26",
     [
       [hdr_non_const_file_path, hdr_non_const],
       [meta_file_non_const_path, self.meta(class_name_non_const,class_def)],
     ]
    ]
  end

  def self.csv(class_name, members, writer)

    var = class_name.camel_case

    [

      "  // Static method to map CSV columns to struct fields",
      "  static #{class_name} CSVType(io::CSVReader<#{members.size}>& reader) {",

      members.map { |m| "    std::string #{m.member_name.camel_case};" }.join("\n") + "\n",

      "    reader.read_row(" + members.map { |m| m.member_name.camel_case }.join(",") + ");\n",

      "    #{class_name} obj (",
      "    " + members.map { |m| "    " + to_convert(m.type, m.member_name.camel_case) }.join(",\n    ") + ");\n",
      "    return obj ;",
      "  }",
    ] * "\n"
  end
end

