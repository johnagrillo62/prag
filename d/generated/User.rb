class User
  attr_reader :id
  attr_accessor :age, :contact, :employer, :projects, :metadata, :investments, :nested

  def initialize
    @projects = []
    @metadata = {}
    @investments = {}
    @nested = {}
  end
end
