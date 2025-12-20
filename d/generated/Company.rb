class Company
  attr_reader :name
  attr_accessor :headquarters, :offices

  def initialize
    @offices = {}
  end
end
