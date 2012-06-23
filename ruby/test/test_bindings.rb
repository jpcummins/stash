require 'test/unit'
require 'stash'

class TypesTest < Test::Unit::TestCase
  class Bar
    def baz
      "3"
    end
  end

  class Foo
    def a
      "1"
    end

    def b
      2
    end

    def c
      Bar.new
    end

  end

  def test_bool
    t = Template.new '{{#a}}1{{/a}}{{^a}}2{{/a}}'
    assert_equal "1", t.render(:a => true)
  end

  def test_object
    t = Template.new '{{#foo}}{{a}}{{b}}{{#c}}{{baz}}{{/c}}{{/foo}}'
    assert_equal "123", t.render(:foo => Foo.new)
  end

  def test_symbol_lookup
    t = Template.new '{{#a}}1{{/a}}{{^a}}2{{/a}}'
    assert_equal "1", t.render(:a => true)
  end

  def test_context_as_object
    t = Template.new '{{a}}'
    context = Foo.new
    assert_equal "1", t.render(context)
  end

  def test_context_as_object_with_nesting
    t = Template.new '{{#c}}{{baz}}{{b}}{{/c}}'
    context = Foo.new
    assert_equal "32", t.render(context)
  end

  def test_context_as_unsupported_type_raises
    t = Template.new '{{a}}'
    context = "a"
    assert_raise ArgumentError do |e, message|
      t.render context
      assert_equal "Expected an object or hash as argument 1", message
    end
  end

  def test_object_context_miss
    t = Template.new '{{#c}}{{a}}{{/c}}'
    context = Foo.new
    assert_equal "1", t.render(context)
  end

  class ContextTest
    attr_accessor :context

    def test
      @context[:a]
    end

    def list
      [ {:a => '1'}, {:a => '2'}, {:a => '3'} ]
    end
  end

  def test_context_variable
    t = Template.new '{{#list}}{{test}}{{/list}}'
    assert_equal "123", t.render(ContextTest.new)
  end

  class HugeTemplate

    def a
      (1..100).to_a
    end

    def b
      "1" * 1000
    end
  end

  def test_very_large_template
    content = ("0" * 1000000) + '{{#a}}{{b}}{{/a}}' + ("3" * 1000000)
    t = Template.new content
    expected = ("0" * 1000000) + ("1" * 1000 * 100) + ("3" * 1000000)
    assert_equal expected, t.render(HugeTemplate.new)
  end

end
