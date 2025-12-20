module Cls
  class Member
    attr_reader :member_name, :column_name, :type, :unique, :access

    def initialize(column_name, member_name, type, unique, access, required)

      if member_name == "class"
        member_name = "cclass"
      end

      if member_name == "export"
        member_name = "cexport"
      end

      if member_name == "short"
        member_name = "cshort"
      end

      @column_name = column_name
      @member_name = member_name
      @type = type
      @unique = unique
      @required = required
    end

    def access
      if @access == nil
        :public
      else
        @access
      end
    end
    
    def required?
      if @required == nil or @required == true or @required == :true
        true
      else
        false
      end
    end
    
    def optional?
      if @required == false or @required == :false
        true
      else
        false
      end
    end
    
    def private
      @access == :private
    end
    
    def public
      @access == nil || access == :public
    end
  end

  class Clss
    attr_reader :class_name, :table_name, :methods, :members
    attr_accessor :package

    def initialize(table_name, class_name, members=[], uniques=[], database_name=[], schema=[])
      @table_name = table_name
      @class_name = class_name
      @members = members
      @uniques = uniques
      @database_name = database_name
      @schema = schema
      
      
    end
      

    def uniques
      if (@uniques == nil)
        return []
      end
      @uniques
    end

    def schema(comment)
      if (@schema == nil)
        return ""
      end
      return @schema.split("\n").reject { |l| l.start_with?("---") }.map { |l| "#{comment} #{l.squish}".strip() } * "\n"
    end

    def getSelectFields
      members.map { |m|
        if m.column_name =~ /\(/
          "\"" + m.column_name + "\""
        else
          m.column_name
        end
      } * ","
    end

    def to_s
      "#{@class_name} #{@members.map { |a| a }}"
    end

    def private_members
      members.inject([]) {|array, member|
        if member.access == :private
          array << member
        end
        array
      }
    end
    
    def public_members
      members.inject([]) {|array, member|
        if member.access == :private
          array << member
        end
        array
      }
    end
  end
end
