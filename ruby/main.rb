require 'date'
class String
  def to_date_mine
    # Try parsing the string with the default format (YYYY-MM-DD)
    begin
      Date.parse(self)
    rescue ArgumentError => e
      # If parsing fails, raise a custom error message
      raise "Invalid date format: #{self}"
    end
  end
end


def to_boolean(str)
  truthy_values = ['true', '1', 'yes', 'y']
  falsy_values = ['false', '0', 'no', 'n']

  return true if truthy_values.include?(str.strip.downcase)
  return false if falsy_values.include?(str.strip.downcase)

  nil  # If it's an unrecognized value
end



2005.upto(2024).reject{|y| y==2020}.each do|y|


  
  
  puts y
end
