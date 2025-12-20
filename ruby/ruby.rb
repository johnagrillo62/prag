require_relative "writer"


 # to_lang
 # to_lang_orm
 #
 #

module Ruby
  @types = Bhw::load_yaml("ruby")
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
     writer = Writer.new(Writers["ruby"])

     class_name = writer.to_class_name(class_def.class_name)
     if class_name[0].match?(/\d/)
       class_name = "T" + class_name
     end

     ext_src = writer.source_file_ext

     pdir = class_def.package * "/"


     src = File.join(pdir, writer.to_file_name(class_def.class_name) + "." + ext_src)

     # does the table have a unique id
     
     indexed = class_def.uniques.select{|n| class_name.downcase == n.downcase}[0]
     if indexed
       indexed = writer.to_member_name(indexed) + "_"
     end


     out = [
       "require 'csv'",       
       class_def.schema("#"),
       "",
       "class #{class_name}",
       "  attr_accessor " + class_def.members.map { |m| ":"+writer.to_member_name(m.member_name)} * ", ",
       "",
       
       self.initialize(class_def.members, writer),
       self.to_csv(class_def.members),
       self.parse_csv_to_list(class_def.members, class_name, writer),
       if indexed
         self.parse_csv_to_list(class_def.members, class_name, writer)
       end
     ]
     
     ["ruby", [[src, out.compact * "\n"]]]
    
   end


   def self.initialize(members, writer)
     [
       "  def initialize(" + members.map { |m| +writer.to_member_name(m.member_name)} * ", " + ")",
       members.map { |m| "    @" + writer.to_member_name(m.member_name) + " = " + writer.to_member_name(m.member_name)},
       "  end",
       "",
       "",
     ] * "\n"
   end

   def self.to_csv(members)
     [
       "  def to_csv",
       "   [",
       members.each_with_index.map{ |m,i|
         if m.type.class.name == "Types::DateTime"
           "    @" + m.member_name.snake_case + '.strftime("%m/%d/%y %H:%M:%S")'
           
         else     
           "    @" + m.member_name.snake_case
         end
         
       }.join(",    \n"),
       "   ].join(\",\")",
       "  end",
       "",
     ] * "\n"
   end

   def self.parse_csv_to_list(members, class_name, writer)
     [       
       "  def self.parse_csv_to_list(filename)",
       "    objs = []",
       "    CSV.foreach(filename, headers: true) do |row|",
       "       objs << " + class_name + ".new(",
       
       members.each_with_index.map{ |m,i|
         
         row = "row['#{m.column_name}']" 
         convert_to = to_convert(m.type)
         convert_to = if convert_to =~  /^\./
                        row + convert_to
                      elsif convert_to == ""
                        row
                      else
                        convert_to + "(" + row + ")"
                      end
         
         "          "+ convert_to + ", # " + writer.to_member_name(m.member_name)
       } * "\n",
       
       
       "     )",
       "    end",
           "    objs",
           "  end",
           "end",
           "",
     ] * "\n"
   end
end


