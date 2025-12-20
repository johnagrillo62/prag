module Types

  module Action
    ToType = :to_type
    FromString = :from_string
  end

  class Integer
    attr_reader :bits

    def initialize(bits)
      @bits = bits
    end
  end


  class Boolean 

  end

  class DateTime 

  end

  class Float 
    attr_reader :type

    def initialize(type)
      @type = type
    end
  end

  class Currency 
  end

  class String 

    attr_reader :len
    def initialize(len)
      @len = 100
    end

  end
end
