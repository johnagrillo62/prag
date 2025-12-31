class String
  def squish
    gsub!(/\A[[:space:]]+/, '')
    gsub!(/[[:space:]]+\z/, '')
    gsub!(/[[:space:]]+/, ' ')
    self
  end

  # replace spaces with "-"
  def dash_case
    self.gsub(" ","-").downcase
  end

  def snake_case
    self.downcase.gsub(" ","_").gsub(/([a-z])_(?=[^a-z]|$)/, '\1')
  end
  
  def down_case
    self.downcase.gsub(" ","").gsub(/([a-z])_(?=[^a-z]|$)/, '\1')
  end

  def camel_capitalize_case
    self.split(" ").map{|s| s.capitalize} * ""
  end

  def camel_case
    s = camel_capitalize_case
    s[0].downcase + s[1..-1]
  end


  
end

class String
  # Method to check if the string is mixed case (contains both upper and lower case letters)
  def mixed_case?
    self.match?(/[a-z]/) && self.match?(/[A-Z]/)
  end

  # Method to convert mixed-case string to space-separated words
  # treating consecutive uppercase as one word and separating at lowercase-uppercase transitions
  def convert_string
    compacted = split.map(&:downcase).reject { |word| word.length == 1 }.join
    remaining_words = split.select { |word| word.length == 1 }
    (compacted + (remaining_words.any? ? ' ' + remaining_words.join(' ') : '')).strip
  end
  
  def mixed_case_to_spaces

    return self unless mixed_case?

    s = self.gsub("_"," ")
    
    s.chars.reduce([]) do |words, char|
      if char == char.upcase && !words.empty?
        words << char
      else
        words.last ? words.last << char : words << char
      end
      words
    end.join(' ').downcase
  end
end






  
