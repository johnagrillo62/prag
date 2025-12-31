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

module Cpp
  @types = Bhw::load_yaml("cpp")

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

  def self.meta(members, class_name)
    out = []
    out << "    static constexpr auto fields = std::make_tuple("
    out << members.map {|m|
      
      attrs = "Getter | Serializable | Hashable"
      if m.unique == true
        attrs = "PrimaryKey | " + attrs
      end


      ["        FieldMeta< ",
       class_name,
       ", ",
       self.to_type(m.type),
       " , ",
       attrs,
       " > {",
       "&",
       class_name,
       "::",
       m.member_name.snake_case,
      "_, \"",
       m.member_name.camel_case,
       "\", \"",
       m.column_name.camel_case,
       "\", \"",
       m.column_name.camel_case,
       "\"",
       "}"
      ] * ""


    } * ",\n"

    out << "    );"

    puts out * "\n"
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
    writer = Writer.new(Writers["cpp"])

    members = class_def.members
    ext_hdr = writer.header_file_ext
    ext_src = writer.source_file_ext

    dir = class_def.package * "/"
    hdr_file_path = File.join(dir, writer.to_file_name(class_def.class_name) + "." + ext_hdr)
    class_name = writer.to_class_name(class_def)
    src_dao_file_path = File.join(dir, writer.to_file_name(class_def.class_name) + "_dao." + ext_src)
    hdr_dao_file_path = File.join(dir, writer.to_file_name(class_def.class_name) + "_dao." + ext_hdr)

    package = class_def.package.map { |c| c.strip } * "::"
    indexed = if class_def.uniques.size == 0
                false
              else
                true
              end

    key = class_def.members.select { |m| m.unique }
    if key.size > 0
      key = writer.to_reader_name(key[0].member_name)
    end

    src = ["",
           "#define WIN32_LEAN_AND_MEAN",
           "// ReSharper disable once CppUnusedInclude",
           "#include <windows.h> // IWYU pragma: keep",
           "",
           "",
           "#include \"#{hdr_file_path}\"",
           "",
           "#include <sqltypes.h>",
           "",
           "#include \"bhw/util.h\"",
           self.write_read(class_def.members, class_def.package, class_name, writer, package),
          ] * "\n"

    hdr = [
      "#pragma once",
      "",
      "#include <chrono>",
      "#include <string>",
      "",
      "#include \"bhw/meta.h\"",
      "",
      "namespace #{package}",
      "{",
      "",
      "class #{class_name} ",
      "{",
      "public:",
      "    #{class_name}() = delete;\n",

      "\n    struct Init {" + members.map do |m|
        ptype = self.to_type(m.type)

        ptype + " " + writer.parameter_str(m.member_name) + ";"
      end * "" + "};",
      "",

      "    static auto from(Init init) -> #{class_name}" +
        "    {",
      "       return #{class_name}{std::move(init)};",
      "    } ",
      "",

      members.map do |m|
        m_name = m.member_name
        m_type = to_type(m.type)
        mname = writer.to_member_name(m_name)
        pname = writer.parameter_str(m_name)
        wstr = writer.to_writer_name(m_name)
        rstr = writer.to_reader_name(m_name)

        retType =
          if m_type =~ /string/
            " -> std::string_view "
          else
            ""
          end

        [
          #"/// #{m.inspect}",
          #"/// #{m.type.inspect}",
          #"  /*************************/",
          #"  void #{wstr}(#{m_type} #{pname}) ",
          #"  {",
          #"    #{mname} = #{pname};",
          #"  }",
          #"",
          "    [[nodiscard]] auto #{rstr}() const noexcept#{retType}{ return " + writer.to_member_name(m_name) + ";" + "} ",

        ] * "\n" + "\n"
      end,
      if (indexed)
        [
          "    [[nodiscard]] auto getKey() const -> uint64_t {return #{key}();}"
        ] * "\n" + "\n"
      end,
      "",
      "private:",

      # R"lit(select RELAY,MEET,LO_HI,TEAM,LETTER,AGE_RANGE,SEX,"ATH(1)","ATH(2)","ATH(3)","ATH(4)","ATH(5)","ATH(6)","ATH(7)","ATH(8)",DISTANCE,STROKE,RELAYAGE from RELAY)lit"; 

      "  explicit #{class_name}(Init init) : " + members.map do |m|

        ptype = self.to_type(m.type)
        if ptype =~ /string/
          writer.to_member_name(m.member_name) + "(std::move(init." + writer.parameter_str(m.member_name) + "))"
        else
          writer.to_member_name(m.member_name) + "(init." + writer.parameter_str(m.member_name) + ")"
        end
      end * ", ",
      "     {}",
      "",

      members.each_with_index.map do |m, i|
        m_type = to_type(m.type)
        mname = writer.to_member_name(m.member_name)
        [
          "  #{m_type} #{mname};",
        ] * "\n"
      end * "\n",
      
      #"public:",
      #"  static constexpr char query[] = R\"lit(select " + class_def.getSelectFields + " from " + class_def.table_name + ")lit\"; ",

      #self.meta(members, class_name),
      "",

      "};\n",
      "} //namespace",
    ]

    hdr_dao = [
      "#pragma once",
      "",
      "#define WIN32_LEAN_AND_MEAN",
      "// ReSharper disable once CppUnusedInclude",
      "#include <windows.h>",
      "",

      class_def.schema("//"),
      "",
      "#include <sqltypes.h>",
      "#include <unordered_map>",
      "#include <vector>",
      "",
      "#include \"#{hdr_file_path}\"",
      "#include \"bhw/util.h\"",
      "",
      "namespace #{package}",
      "{",
      "",

      "using #{class_name}Vec = std::vector<std::unique_ptr<#{package}::#{class_name}>>;",

      "using #{class_name}UniqueMap = std::unordered_map<uint64_t, std::unique_ptr<#{package}::#{class_name}>>;",
      "using #{class_name}PtrMap = std::unordered_map<uint64_t, #{package}::#{class_name}*>;",
      "using #{class_name}UniqueMapUnique = std::unique_ptr<std::unordered_map<uint64_t, std::unique_ptr<#{package}::#{class_name}>>>;",
      "using #{class_name}ObjMap = std::unordered_map<uint64_t, #{package}::#{class_name}>;",
      "",

      "class #{class_name}Dao",
      "{",
      "public:",

      "",
      "  [[nodiscard]]",
      "  static auto getVec(const SQLHANDLE& conn)",
      "  {",
      "    return bhw::FetchRows<#{class_name}Vec, #{class_name}>(conn, query);",
      "  }",
      "",
      if indexed
        [

          "  [[nodiscard]]",
          "  static auto getMap(const SQLHANDLE& conn)",
          "  {",
          "    return bhw::FetchRows<#{class_name}UniqueMap, #{class_name}>(conn, query);",
          "  }",
          "",
          "  [[nodiscard]]",
          "  static auto getObjMap(const SQLHANDLE& conn)",
          "  {",
          "    return bhw::FetchRowsObj<#{class_name}ObjMap, #{class_name}>(conn, query);",
          "  }",
          "",

          "  [[nodiscard]]",
          "  static auto getUniqueMap(const SQLHANDLE& conn)",
          "  {",
          "    return bhw::FetchRowsUnique<#{class_name}UniqueMap, #{class_name}>(conn, query);",
          "  }",
          "",
          "  [[nodiscard]]",
          "  static auto getObjMapUnique(const SQLHANDLE& conn)",
          "  {",
          "    return bhw::FetchRowsObjUnique<#{class_name}UniqueMap, #{class_name}>(conn, query);",
          "  }",
          "",
          "private:",
          "  static constexpr char query[] = R\"lit(select " + class_def.getSelectFields + " from " + class_def.table_name + ")lit\"; ",
          "};",
          "}"
        ] * "\n"
      else
        ""
      end,
    ]

    ["cpp",
     [
       [hdr_file_path, hdr],
       [src_dao_file_path, src],
       [hdr_dao_file_path, hdr_dao],
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

