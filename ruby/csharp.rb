
# to_lang
# to_lang_orm
#
#

module Types
  class Integer
    def to_csharp
      if (@bits == 64)
        "long"
      elsif (@bits == 32)         
        "int"
      elsif (@bits == 16)         
        "short"
      else
        "byte"        
      end
    end

    def to_csharp_orm
      if (@bits == 64)       
        "GetInt32"  # ??? GetInt64 seems to be broken
      elsif (@bits == 32)         
        "GetInt32"
      elsif (@bits == 16)         
        "GetInt16"
      else
        "GetByte"        
      end
    end
  end

  class String
    def to_csharp
      "string"
    end

    def to_csharp_orm
      "GetString"
    end
  end

  
  class Float
    def to_csharp
      if (@type == :single)
        "float"
      else
        "double"
      end
    end

    def to_csharp_orm
      if (@type == :single)
        "GetFloat"
      else
        "GetDouble"
      end

    end
    
  end

  class DateTime
    def to_csharp
      "DateTime"
    end
    def to_csharp_orm
      "GetDateTime"
    end
  end

  class Boolean
    def to_csharp
      "bool"
    end
    def to_csharp_orm
      "GetBoolean"
    end
  end

  class Currency
    def to_csharp
      "decimal"
    end
    
    def to_csharp_orm
      "GetDecimal"
    end
  end
end




module CSharp
  def self.write(class_def)
    writer = Writer.new(Writers["csharp"])

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

    getDef = if (indexed)
               "static public Dictionary<long,#{class_name}> GetBy(OdbcConnection connection)"
             else
               "static public List<#{class_name}> Get(OdbcConnection connection)"
             end

    getVec = if (indexed)
               "var collection = new Dictionary<long,#{class_name}>();"
             else
               "var collection = new List<#{class_name}>();"
             end
    
    out = [
      "using System;",
      "using System.Collections.Generic;",
      "using System.Data.Odbc;",
      "namespace db",
      "{",
      "namespace #{class_def.package}",
      "{",
      class_def.schema("//"),
      "",
      "public class #{class_name}(" + class_def.members.map do |m|
        
        nullable = ""
        if m.column_name.downcase == class_name.downcase
          
        elsif (m.type.class != Types::String) and m.optional?
          nullable = "?"
        end
        m.type.to_csharp + "#{nullable} " + writer.parameter_str(m.column_name)
      end * ", " + " )",
      "{",
      
      getDef,
      "{",
      "  " + getVec,
      "  string query = \"select * from #{class_def.table_name}\";",
      "  using (OdbcCommand command = new OdbcCommand(query, connection))",
      "  using (OdbcDataReader reader = command.ExecuteReader())",
      "{",
      "  while (reader.Read())",
      "  {",
      "      ///",
      "      /// GetXX has to called in the same order as the sql order.",
      "      /// Cant make these arguments to a constructor, since the order is indeterminate.",
      "      ///",
      
      class_def.members.each_with_index.map do |m,i|
        
        nullable = 
          if m.column_name.downcase == class_name.downcase
            false
          elsif m.optional?
            true
          else
            false
          end
        
        assign = "  var arg#{i} = "
        if (nullable)
          if m.type.class == Types::String
            assign += "reader.IsDBNull(#{i}) ? (#{m.type.to_csharp}?)null :"
          else
            assign += "reader.IsDBNull(#{i}) ? (#{m.type.to_csharp})null :"
          end
        end
        
        assign += "reader." + m.type.to_csharp_orm + "(" + (i).to_s
        
        [
          assign + ");"   + " /* #{m.column_name} nullable #{nullable}*/",
          ##"Console.WriteLine(\"#{m.column}\");"
        ] * "\n"
        
      end,
      
      if (indexed)
        "      var obj = new #{class_name}(" +  class_def.members.each_with_index.map do |m,i|
          "arg#{i}"
        end * ", " + ");" + "\n      collection[obj.#{indexed}] = obj;"
      else
        "      collection.Add( new #{class_name}(" +  class_def.members.each_with_index.map do |m,i|
          "arg#{i}"
        end * ", " + "));"
      end,
      
      
      "    }",
      "  }",
      "  return collection;",
      "}",
      
      
      "  // Properties",

      class_def.members.map do |m|
        m_type = m.type.to_csharp

        
        if m.column_name.downcase == class_name.downcase
          
        elsif (m.type.class != Types::String) and m.optional?
          m_type += "?"
        end
        
        mname = writer.to_member_name(m.column_name)
        pname = writer.parameter_str(m.column_name)

        if indexed
          # member names cannot be the same as their enclosing type	

          if m.column_name.downcase == mname.downcase
            mname += "_"
            print mname, "\n"
            
          end
        end
        
        "  public #{m_type} #{mname}{ get; } = #{pname};"
      end * "\n",
      
      
      # "  // Constructor",
      # "  #{class_name}(" + c.members.map do |m|
      #   nullable = ""
      #   if m.name.downcase == c.name.downcase
      #   elsif (m.type.class != Types::String) and m.nullable
      #     nullable = "?"
      #   end
      #   m.type.to_csharp + "#{nullable} " + writer.parameter_str(m.name)
      # end * ", " + " )",
      # "  {",
      # c.members.map do |m|
      #   "    " + writer.member_str(m.name) + " = " + writer.parameter_str(m.name) + ";"
      # end * "\n",
      # "  }",
      "}",
      "}",
      "}"
    ]

    ["csharp", [[src, out.compact * "\n"]]]
    
  end
end


