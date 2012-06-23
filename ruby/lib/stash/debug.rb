class Template::Callback
  attr_accessor :name
  attr_accessor :time

  def initialize(name, time)
    @name = name
    @time = time
  end

  def to_s
    "['#{@name}', #{@time}],"
  end
end

class Template::Debug
  attr_accessor :total_time
  attr_accessor :callbacks

  def initialize
    @callbacks = []
  end

  def log_callback(name, time)
    @callbacks.push Template::Callback.new(name, time)
  end

  def slowest_callbacks(n)
    @callbacks.sort { |a, b| b.time <=> a.time}.first(n)
  end

  def group_slowest_callbacks(n)
    @callbacks.group_by(&:name) \
              .map { |k, v| [ k, v.inject(0) { |sum, x| sum + x.time }, v.length ] } \
              .sort { |a, b| b[1] <=> a[1] } \
              .first(n)
  end

  def mustache_time
    total_time - ruby_time
  end

  def ruby_time
    @callbacks.inject(0) { |sum, x| sum + x.time }
  end

end