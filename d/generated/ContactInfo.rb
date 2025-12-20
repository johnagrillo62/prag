class Contactinfo
  attr_accessor :phone, :address, :previousAddresses

  def initialize
    @previousAddresses = []
  end
end
