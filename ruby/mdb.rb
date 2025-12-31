#
# Parse output of mdb schema
#
# CREATE TABLE [Athlete]
#  (
#       [Athlete]               Long Integer, 
#       [Team1]                 Long Integer, 
#       [Team2]                 Long Integer, 
#       [Team3]                 Long Integer, 
#       [Group]                 Text (6), 
#       [SubGr]                 Text (6), 
#       [Last]                  Text (40), 
#       [First]                 Text (40), 
#       [Initial]               Text (2), 
#       [Sex]                   Text (2), 
#       [Birth]                 DateTime, 
#       [Age]                   Integer, 
#       [Class]                 Text (6), 
#       [ID_NO]                 Text (34), 
#       [Citizen]               Text (6), 
#       [Inactive]              Boolean NOT NULL, 
#       [Pref]                  Text (40), 
#       [Batch]                 Integer, 
#       [WMGroup]               Text (6), 
#       [WMSubGr]               Text (6), 
#       [BCSSASwimmer]          Text (4), 
#       [BCSSADiver]            Text (4), 
#       [BCSSASyncro]           Text (4), 
#       [BCSSAPolo]             Text (4), 
#       [TheSort]               Long Integer, 
#       [DiveCertified]         Boolean NOT NULL, 
#       [DateClubJoined]        DateTime, 
#       [DateGroupJoined]       DateTime, 
#       [AWRegType]             Text (2), 
#       [RegYear]               Integer, 
#       [Foreign]               Boolean NOT NULL, 
#       [ForeignCitizenOf]      Text (6), 
#       [LastUpdated]           DateTime, 
#       [PC_Hide]               Boolean NOT NULL
# );







module MDB

  Rename = {
    "I_R" => "ind or real",
    "F_P" => "f p",
    "Mtevent" => "mt event",
    "MTEVENT" => "mt event",
    "MTEVENTE" => "mt evente",    
    "DQCODE" => "dq code",
    "DQDESCRIPT" => "dq descript",
    "DQCODESecondary" => "dq code secondary",
    "DQDESCRIPTSecondary" => "dq descript secondary",
    "RELAYAGE" => "relay age",    
  }

  MapTypes = {
    "Long"     => ->(a) {Types::Integer.new(64)},
    "Integer"  => ->(a) {Types::Integer.new(16)},
    "Byte"     => ->(a) {Types::Integer.new(8)},
    "DateTime" => ->(a) {Types::DateTime.new()},
    "Boolean"  => ->(a) {Types::Boolean.new()},
    "Single"   => ->(a) {Types::Float.new(:single)},
    "Double"   => ->(a) {Types::Float.new(:double)},
    "Currency" => ->(a) {Types::Currency.new()},  
    #(100)
    "Text"     => ->(a) {Types::String.new(a[1][1..-1].to_i)},
  }

  def self.parse_type(words)
    MapTypes[words[0]][words]
  end
  
  def self.readTable(database_name, schema)

    iter = schema.split("\n").each

    uniques = []
  
    begin 
      while true
        s = iter.next
        if s =~ /CREATE TABLE/
          _,_,name = s.split(' ')
          table_name = name[1..-2]
          class_name = table_name
          s = iter.next.squish
          if s == "("
            members = []
            while true
              s = iter.next.squish

              if s=~/^#/
                next
              end
              
              if s != ");"
                
                required = false

                if s =~ /NOT NULL/
                  required = true
                end

                column_name,type = s.split(" ",2)
                column_name = column_name[1..-2]
                member_name = column_name
                
                if Rename.include?(member_name)
                  member_name = Rename[member_name]
                else
                  member_name = column_name.gsub("_"," ").mixed_case_to_spaces.gsub("(","_").gsub(")","_")

                end

                member_name = member_name.downcase.squish

                if Rename.include?(class_name)
                  class_name = Rename[class_name]
                end
                
                
                unique = 
                  if class_name.downcase == column_name.downcase
                    uniques << name
                    required = true
                    true
                  else
                    false
                  end
                print member_name, " ", required, "\n"
                members << Cls::Member.new(column_name,
                                           member_name,
                                           parse_type(type.gsub(",","").split(" ")),
                                           unique,
                                           :public,
                                           required
                                          )
              else
                return Cls::Clss.new(table_name, class_name, members, uniques, database_name, schema)
              end
            end
          end
        end
      end
    rescue StopIteration => e
    end
    
    return Cls::Clss.new("", [])
  end
  

end

