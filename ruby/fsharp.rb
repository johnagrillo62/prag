
#
#
# .net uses 32 bits for LongIntegers
#
#

module Types
  class Integer
    def to_fsharp
      if @bits == 64
        "int32"
      elsif @bits == 32
        "int32"
      elsif @bits == 16
        "int16"
      else
        "byte"        
      end
    end

    
    def to_fsharp_orm
      if @bits == 64
        "GetInt32"
      elsif @bits == 32
        "GetInt32"
      elsif @bits == 16
        "GetInt16"
      else
        "GetByte"        
      end
      
    end
    
  end
  
  class Boolean
    def to_fsharp
      "bool"
    end
    def to_fsharp_orm
      "GetBoolean"
    end
  end
  
  class DateTime
    
    def to_fsharp
      "DateTime"
    end
    
    def to_fsharp_orm
      "GetDateTime"
    end
  end
  
  class Float
    def to_fsharp
      if @type == :single
        "float32"
      else
        "float"
      end
    end
    
    def to_fsharp_orm
      if @type == :single
        "GetFloat"
      else
        "GetDouble"
      end
    end
  end
  
  class Class
  end
  
  class Currency
    def to_fsharp
      "decimal"
    end
    def to_fsharp_orm
      "GetDecimal"
    end
  end
  
  class String

    def to_fsharp
      "string"
    end
    def to_fsharp_orm
      "GetString"
    end
  end

end




module FSharp

  ##################################
  #
  # reserved words in fsharp
  #
  ###################################
  
  def self.fix_reserved(name)
    if name == "member"
      "mmember"
    elsif name == "rec"
      "rrec"
    elsif name.downcase == "type"
      "ttype"
    elsif name == "start"    
      "sstart"
    elsif name == "end"    
      "eend"
    else
      name
    end  
  end
  

  def self.to_parameter(writer, member)
    [
      writer.parameter_str(self.fix_reserved(member.column_name)),
      " : ",
      member.type.to_fsharp,
      if member.optional
        " option"
      else
        ""
      end
    ] * ""
  end

  
  def self.get_func(writer, class_name, table_name, members)
    [
      "  static member Get (connection: OdbcConnection) =",
      "    let query = \"SELECT * FROM #{table_name}\"",
      "    use command = new OdbcCommand(query, connection)",
      "    use reader = command.ExecuteReader()",
      "    let objs = ",
      "      seq {",
      "        while reader.Read() do",
      members.each_with_index.map do |m,i|
        mname = writer.to_member_name(m.column_name)
        mtype = m.type
        [
          if m.type.nullable
            #"         let arg#{i} = if reader.IsDBNull(#{i}) then None else Some(reader.#{mtype.to_fsharp_orm}(#{i}))  "

            "         let arg#{i} = #{mtype.to_fsharp_orm}Option reader #{i}"
          else
            "         let arg#{i} = reader.#{mtype.to_fsharp_orm}(#{i})  "
          end + " // #{m.column_name} == #{writer.to_member_name(m.column_name)}"
        ] * "\n"
      end,
      
      "         yield #{class_name}(" + members.each_with_index.map do |m,i|  "arg#{i}"  end * ", " + ")",
      
      "       } |> Seq.toList",
      "    objs",
    ] * "\n"
  end
  
  
  def self.get_by_func(writer, class_name, table_name, members, uniques)
    uniques = []
    uniques.map do |u| 
      [
        "  static member GetBy#{u} (connection: OdbcConnection) : Map<int32, #{class_name}> =",
        "    let query = \"SELECT * FROM #{table_name}\"",
        "    use command = new OdbcCommand(query, connection)",
        "    use reader = command.ExecuteReader()",


        "    let objs = ",
        "      seq {",
        "        while reader.Read() do",
        members.each_with_index.map do |m,i|
          mname = writer.to_member_name(m.column_name)
          mtype = m.type
          [
            if m.type.nullable
              "          let arg#{i} = #{mtype.to_fsharp_orm}Option reader #{i}  "
            else
              "          let arg#{i} = reader.#{mtype.to_fsharp_orm}(#{i})  "
            end + " // #{m.column_name} == #{writer.to_member_name(m.column_name)}"
          ] * "\n"
        end,
        
        "          yield #{class_name}(" + members.each_with_index.map do |m,i|  "arg#{i}"   end * ", " + ")",
        
        "       } |> Seq.toList",
        "       // Convert to a dictionary indexed by #{u}",
        "    objs |> Seq.fold (fun acc obj ->",
        "        acc.Add(obj.#{writer.to_member_name(u)}, obj)",
        "      ) Map.empty"
      ] * "\n"
    end * "\n"
  end

  def self.write(class_def)
    writer = Writer.new(Writers["fsharp"])

    pdir = (class_def.package.map{|a| a.downcase} * "/")

    mod = class_def.package.map{|a| a.camel_capitalize_case} * "."

    class_name = writer.to_class_name(class_def.class_name)


    ext_src = writer.source_file_ext
    src = File.join(pdir, writer.to_file_name(class_def.class_name) + "." + ext_src)

    out = [


      "module #{mod}",
      "",
      
      "type #{class_name} = {",
      class_def.members.map do |m|
        "    " + fix_reserved(writer.to_member_name(m.column_name)) + " : " + m.type.to_fsharp
      end * "\n",
      "}",
    ]
    [ "fs" , [[src, out.compact * "\n"]]]
  end
end

