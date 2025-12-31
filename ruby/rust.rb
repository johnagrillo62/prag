require_relative "writer"

module Types
  class Integer
    def to_rust(required = true)

      bits = 
        if @bits == 64
          "u64"
        elsif @bits == 32
          "u32"
        elsif @bits == 16
          "u16"
        else
          "u8"
        end

      if required
        "Option<#{bits}>"
      end
      "Option<#{bits}>"
      
    end

    def to_rust_orm
      if @bits == 64
        "read_u64"
      elsif @bits == 32
        "read_u32"
      elsif @bits == 16
        "read_u16"
      else
        "read_u8"
      end
    end
  end

  class Boolean
    def to_rust(required = true)
      "bool"
    end

    def to_rust_orm
      "read_bool"
    end
  end

  class DateTime

    def to_rust(required = true)
      "NaiveDateTime"
    end

    def to_rust_orm
      "read_datetime"
    end
  end

  class Float
    def to_rust(required = true)
      if @type == :single
        "f32"
      else
        "f64"
      end
    end

    def to_rust_orm
      if @type == :single
        "read_f32"
      else
        "read_f64"
      end
    end
  end

  class Class
  end

  class Currency
    def to_rust(required = true)
      "Decimal"
    end

    def to_rust_orm
      "read_decimal"
    end
  end

  class String
    def to_rust(required = true)
      if (required)
        "String"
      else
        "Option<String>"
      end
    end

    def to_rust_orm
      "read_string"
    end

  end
end

def member_name(name)
  if name == "type"
    "ttype"
  else
    name
  end
end

module Rust

  def self.write(class_def)

    writer = Writer.new(Writers["rust"])

    ext = writer.source_file_ext

    src_dir = class_def.package * "/"
    src_file = File.join("src", src_dir, writer.to_file_name(class_def))
    ""
    return ["rust", [
      [src_file + "." + ext, self.write_rust(writer, class_def)],
      [src_file + "_csv." + ext, self.write_rust(writer, class_def, :csv)],
      [src_file + "_dao." + ext, self.write_rust(writer, class_def, :dao)],
    ]]
  end

  def self.write_rust(writer, class_def, feature = :none)

    class_name = writer.to_class_name(class_def)
    class_name = "RResult" if class_name == "Result"
    
    imports = Set.new()
    templates = Set.new()
    impltemplates = Set.new()
    class_def.members.map do |m|
      if m.type.class == Types::Currency
        imports << "rust_decimal::Decimal"
      end
      if m.type.class == Types::DateTime
        #imports << "chrono::{DateTime,TimeZone}"
        #templates << "Tz : TimeZone"
        #impltemplates << "Tz"
      end
    end

    templates = if templates.size == 0
                  ""
                else
                  "<" + templates.map { |a| a } * "," + ">"
                end

    impltemplates = if impltemplates.size == 0
                      ""
                    else
                      "<" + impltemplates.map { |a| a } * "," + ">"
                    end

    out = [
      if feature == :dao
        class_def.schema("//")
      end,
      imports.map { |i| "use #{i};" } * "\n",
      if feature == :none
        [
          "#[derive(serde::Serialize, serde::Deserialize,Debug)]",
          "#[serde(rename_all = \"UPPERCASE\")]",
        ] * "\n"
      else
        [
         "use crate::" + class_def.package * "::" + "::" +  writer.to_file_name(class_def) + "::" +  class_name + ";"
          
        ]
      end,

      ##[serde(rename = "ATHLETE")]
      if feature == :none
        [
          "pub struct #{class_name}#{templates} {",
          class_def.members.map do |m|
            to_member(writer, m, feature)
          end * ",\n",
          "}",
        ] * "\n"
      end,
      "impl#{impltemplates} #{class_name}#{impltemplates} {",
      if feature == :none
        [
          "",
          "// Constructor to create a new Ingredient",
          "    pub fn new(" + class_def.members.map { |m|
            "#{writer.to_member_name(m)} : #{m.type.to_rust(m.required?)}"
          } * ", " + ") -> Self{",
          "    #{class_name} {",
          class_def.members.map { |m|
            "        #{writer.parameter_str(m)}"
          } * ",\n",
          "    }",
          "}",
        ] * "\n"
      end,
        

      if feature == :csv
        [
           "",
           "    fn from_csv_path(csv: String) -> std::result::Result<Vec<#{class_name}>, Box<dyn std::error::Error>> {",
           "       let mut vec: Vec<#{class_name}> = Vec::new();",
           "       let mut rdr = csv::ReaderBuilder::new().has_headers(true).from_path(csv)?;",
           "       for elem in rdr.deserialize() {",
           "            let obj: #{class_name} = elem?;",
           "            vec.push(obj);",
           "       }",
           "       Ok(vec)",
           "    }",
        ] * "\n"
      end,
      if feature == :dao
        [

        "    fn from_mdb(conn: Connection) -> Result<Vec<#{class_name}>, Box<dyn Error>> {",
        "        let mut vec: Vec<#{class_name}> = Vec::new();",
        "        match conn.execute(\"#{class_def.getSelectFields} FROM #{class_def.table_name}\", (), None)? {",
        "            Some(mut cursor) => {",
        "                let mut buffers = TextRowSet::for_cursor(BATCH_SIZE, &mut cursor, Some(4096))?;",
        "                let mut row_set_cursor = cursor.bind_buffer(&mut buffers)?;",
        "                while let Some(_batch) = row_set_cursor.fetch()? {",
        "                    for row_index in 0.._batch.num_rows() {",
        class_def.members.each_with_index.map do |m,i|
          mname = member_name(writer.to_member_name(m))
          mtype = m.type
          [
            "                        let #{mname} = #{mtype.to_rust_orm()}(_batch.at(#{i}, row_index))?;"
          ] 
        end * "\n",
        "                        let obj = #{class_name} {",
        
        class_def.members.each_with_index.map do |m,i|


          mname = member_name(writer.to_member_name(m))
          mtype = m.type
          [
            if m.optional?
              "                          #{mname} : Some(#{mname})"
            else
              "                            #{mname}"
            end
          ]
        end * ",\n",
        
        "                        };",
        "                        vec.push(obj);",
        "                    };",
        "                }",
        "            }",
        "	    None => todo!()",
        "        }",
        "        Ok(vec)",
        "    }",
        ] * "\n"
      end,

      if feature == :none
        class_def.members.each_with_index.map { |m, i|
          mname = member_name(writer.to_member_name(m))
          if m.private
            [
              "    pub fn #{mname}(&self) -> #{m.type.to_rust} {",
              "        self.#{mname}",
              "    }",
              "",
              
              "    pub fn set_#{mname}(&mut self, #{writer.parameter_str(m)} : #{m.type.to_rust}) {",
              "        self.#{mname} = #{mname}",
              "    }",
              "",
              
            ] * "\n"
          end
        }.compact
      end,
      
      "}",
    ]

    out.compact * "\n"
  end

  def self.to_member(writer, m, feature = :none )
    out = []

    if m.optional?
      out <<
        if m.type.class == Types::DateTime
          "    #[serde(deserialize_with = \"deserialize_optional_datetime\")]\n"
        else
          ""
        end +
        "    #[serde(default)] " + member_name(writer.parameter_str(m.member_name)) + " : Option<" + m.type.to_rust + ">"
      
    else
      out <<
        if m.type.class == Types::Boolean
          "    #[serde(deserialize_with = \"deserialize_bool_from_0_1\")]\n"
        else
          ""
          ##[serde(rename = "ATHLETE")]
        end + "    #[serde(rename = \"#{m.column_name}\")] pub " + member_name(writer.parameter_str(m.member_name)) + " : " + m.type.to_rust
    end
    out * "\n"
  end
end
