require_relative "writer"
require 'set'

def check_optional_go(str, required)
  if required
    "#{str}"
  else
    "*#{str}"
  end
end

module Types
  
  class Integer
    def to_go(required = true)
      if (@bits == 64)
        check_optional_go("uint64", required)
      elsif (@bits == 32)
        check_optional_go("uint32", required)
      elsif (@bits == 16)
        check_optional_go("uint16", required)
      else
        check_optional_go("uint16", required)
      end
    end

    def to_go_orm
      if (@bits == 64)
        "getLong"
      elsif (@bits == 32)
        "getInt"
      elsif (@bits == 16)
        "getShort"
      else
        "getChar"
      end

    end

  end

  class Boolean
    def to_go(required = true)
      check_optional_go("bool", required)
    end

    def to_go_orm
      "getBool"
    end
  end

  class DateTime

    def to_go(required = true)
      check_optional_go("time.Time", required);
    end

    def to_go_orm
      "getDateTime"
    end
  end

  class Float
    def to_go(required = true)
      if (@type == :single)
        check_optional_go("float32", required)
      else
        check_optional_go("float64", required)
      end
    end

    def to_go_orm
      if (@type == :single)
        "getFloat"
      else
        "getDouble"
      end
    end
  end

  class Class
  end

  class Currency
    def to_go(required = true)
      check_optional_go("float32", required)
    end

    def to_go_orm
      "getCurrency"
    end
  end

  class String
    def to_go(required = true)
      check_optional_go("string", required)
    end

    def to_go_orm
      "getText"
    end
  end
end

module Go

  def self.write(class_def)
    writer = Writer.new(Writers["go"])
    class_name = writer.to_class_name(class_def.class_name)
    ext_src = writer.source_file_ext


    dir = class_def.package * "/"
    src = dir + "/" + writer.to_file_name(class_def.class_name) + "." + ext_src

    imports = Set.new
    imports.add("import \"database/sql\"")
    class_def.members.map do |m|
      if m.type.class == Types::DateTime
        imports.add "import \"time\""
      elsif m.type.class == Types::String
        imports.add "import \"gomanager/common\""
      end
    end

    indexed = if class_def.uniques
                class_def.uniques[0]
                # does the table have a unique id
              else
                false
              end

    out = []
    out << [
      if class_def.package
        "package #{class_def.package.last}"
      else
        "package main"
      end,
      class_def.schema("//"),
      "",
      #imports.to_a * "\n",
      "",

      #"csv",
      
      "type #{class_name} struct",
      "{",
      #class_def.members.map do |m|
      #  "    " + writer.member_str(m.member_name) + " " + m.type.to_go + " `csv:\"#{m.column_name}\"`"
      #end * "\n",
      #"}",

      class_def.members.map do |m|
        mname = writer.to_member_name(m)
        mname,comment = if m.private == true
                          [mname[0].downcase + mname[1..-1], " // private"]
                        else
                          [mname, ""]
                        end
          "    " + mname + " " + m.type.to_go(m.required?) + comment
        
      end * "\n",
      "}",
      "",

      "  func New#{class_name}(" +  class_def.members.map do |m|
        writer.parameter_str(m.member_name) + " " + m.type.to_go(m.required?) 
      end * ", " +  ") (*#{class_name}, error) {",
      "    return &#{class_name} {",

      class_def.members.map do |m|
        mname = writer.to_member_name(m.member_name)
        mname,comment = if m.private == true
                          [mname[0].downcase + mname[1..-1], " // private"]
                        else
                          [mname, ""]
                        end
        "        " + mname + " : " + writer.parameter_str(m.member_name) + "," 
      end * "\n",
     "    }, nil",
      "}",

      class_def.members.map do |m|
        param_name = writer.parameter_str(class_name)
        member_name = writer.to_member_name(m.member_name)
        func_name = writer.to_member_name(m.member_name)

        set_name = writer.parameter_str(m.member_name)
        
        if m.private == true
          member_name = member_name[0].down_case + member_name[1..-1]
          [
            "func (#{param_name} #{class_name}) #{func_name}() #{m.type.to_go(m.required?)} {",
            "  return #{param_name}.#{member_name}",
            "}",
            "",
            "func (#{param_name} * #{class_name}) Set#{func_name}(#{m.type.to_go(m.required?)} #{set_name}) #{m.type.to_go(m.required?)} {",
            "  #{param_name}.#{member_name} = #{member_name}",
            "}",
            "",
          ] * "\n"
        end
      end


      
      # function("Get#{class_name}", ["db *sql.DB"], "[]#{class_name}, error") do
      #   [
      #     "  var results [] #{class_name}",
      #     "",
      #     "  rows, err := db.Query(\"SELECT #{class_def.getSelectFields()} FROM #{class_name}\")",
      #     "",
      #     "  if err != nil {",
      #     "    return results, err",
      #     "   }",
      #     "  defer rows.Close()",
      #     "",
      #     "  for rows.Next() {",
      #     "  obj, err := scan#{class_name}Row(rows)",
      #     "    if err != nil {",
      #     "        return results,err",
      #     "    }",
      #     "    results = append(results, obj)",
      #     "  }",
      #     "  return results, nil"
      #   ] * "\n"
      # end,

      # if indexed
      #   [
      #     "func Get#{class_name}Map(db *sql.DB) (map[uint64]#{class_name}, error) {",
      #     "  results := make(map[uint64]#{class_name})",
      #     "",
      #     "  rows, err := db.Query(\"SELECT #{class_def.getSelectFields()} FROM #{class_name}\")",
      #     "",
      #     "  if err != nil {",
      #     "    return nil, err",
      #     "  }",
      #     "  defer rows.Close()",
      #     "",
      #     "  for rows.Next() {",
      #     "    obj, err := scan#{class_name}Row(rows)",
      #     "    if err != nil {",
      #     "        return results,err",
      #     "    }",
      #     "    results[obj.#{class_name}] = obj",
      #     "  }",
      #     "  return results, nil",
      #     "}",
      #   ] * "\n"
      # end,

      # "func scan#{class_name}Row(rows *sql.Rows)(#{class_name}, error) {",
      # "    var obj #{class_name}",
      # "    if err := rows.Scan(" + class_def.members.each_with_index.map do |m, i|
      #   mname = writer.member_str(m.member_name)
      #   [
      #     "&obj.#{mname}"
      #   ]
      # end * "," + "); err != nil {\n        return obj, err\n    }",
      # "    return obj,nil",
      # "}"
    ]

    ["go", [[src, out.compact * "\n"]]]
  end

  #def self.function(name, args, ret)
  #  "func #{name}(" + args * "," + ") (" + ret + ") {\n" + yield + "\n}"
  #end

end
