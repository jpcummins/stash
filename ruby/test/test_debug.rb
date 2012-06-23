require 'test/unit'
require 'stash'

class TypesTest < Test::Unit::TestCase

  def test_debugging
    t = Template.new '{{#a}}1{{/a}}{{^a}}2{{/a}}'
    debug = Template::Debug.new
    t.render({:a => true}, {}, debug)
    assert debug.total_time > 0
  end

  class Slow
    def a
      sleep 0.01
      "a"
    end
    def b
      sleep 0.1
      "b"
    end
  end

  def test_callback_times
    t = Template.new '{{a}}{{a}}'
    debug = Template::Debug.new
    t.render(Slow.new, {}, debug)
    assert debug.total_time >= 0.02
    assert debug.callbacks.length == 2

    debug.callbacks.each do |callback|
      assert_equal :a, callback.name
      assert callback.time >= 0.01
    end
  end

  def test_slowest_callbacks
    t = Template.new '{{a}}{{b}}{{a}}'
    debug = Template::Debug.new
    t.render(Slow.new, {}, debug)
    assert_equal :b, debug.slowest_callbacks(2)[0].name
    assert_equal :a, debug.slowest_callbacks(2)[1].name
  end

  def test_group_slowest_callbacks
    t = Template.new '{{a}}{{b}}{{a}}'
    debug = Template::Debug.new
    t.render(Slow.new, {}, debug)

    slowest = debug.group_slowest_callbacks(2)[0]
    assert_equal :b, slowest[0]
    assert slowest[1] >= 0.1
    assert_equal 1, slowest[2]

    next_slowest = debug.group_slowest_callbacks(2)[1]
    assert_equal :a, next_slowest[0]
    assert next_slowest[1] >= 0.02
    assert_equal 2, next_slowest[2]
  end

  def test_ruby_time
    t = Template.new '{{a}}{{a}}'
    debug = Template::Debug.new
    t.render(Slow.new, {}, debug)
    assert debug.ruby_time >= 0.02
  end

end
