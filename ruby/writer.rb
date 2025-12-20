class Writer
  attr_reader :header_file_ext, :source_file_ext, :directory
  
  def initialize(config)
    @config = config
    @header_file_ext = config["header_file_ext"]
    @source_file_ext = config["source_file_ext"]    
    @conventions = config["conventions"]
    @directory = config["directory"]
    
    @file = @conventions["file"]
    @class = @conventions["class"]
    @member = @conventions["member"]
    @reader = @conventions["reader"]
    @writer = @conventions["writer"]
    @parameter = @conventions["parameter"]

  end
  
  def to_file_name(arg)
    name = if arg.class == String
             arg
           elsif arg.class == Cls::Clss
             arg.class_name
           end
    name.send(@file["case"] + "_case")
  end

  def to_class_name(arg)
        name = if arg.class == String
             arg
           elsif arg.class == Cls::Clss
             arg.class_name
           end
    name.send(@class["case"] + "_case")
  end

  def to_member_name(arg)
    name = if arg.class == String
             arg
           elsif arg.class == Cls::Member
             arg.member_name
           end
    [
      @member["prefix"],
      name.send(@member["case"] + "_case"),
      @member["suffix"],
    ].compact * ""
  end

  def to_reader_name(arg)
    name = if arg.class == String
             arg
           elsif arg.class == Cls::Member
             arg.member_name
           end
    [
      @reader["prefix"],
      name.send(@reader["case"] + "_case"),
      @reader["suffix"],
    ].compact * ""
  end

  def to_writer_name(arg)
    name = if arg.class == String
             arg
           elsif arg.class == Cls::Member
             arg.member_name
           end
    [
      @writer["prefix"],
      name.send(@writer["case"] + "_case"),
      @writer["suffix"],
    ].compact * ""
  end

  def parameter_str(arg)
    name = if arg.class == String
             arg
           elsif arg.class == Cls::Member
             arg.member_name
           end
    
    name.send(@parameter["case"] + "_case")
  end
end



  
