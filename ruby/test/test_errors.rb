require 'test/unit'
require 'lib/stash'

class ErrorTest < Test::Unit::TestCase

  def test_invalid_section_start
    assert_raise SyntaxError do |e, message|
      t = Template.new '{{_i}}test{{/i}}'
      assert_equal "Expected identifier", message
    end
  end

  def test_raw_variable_without_identifier
    assert_raise SyntaxError do |e, message|
      t = Template.new '{{{>this_is_just_wrong}}}'
      assert_equal "Expected identifier", message
    end
  end

  def test_section_without_identifier
    assert_raise SyntaxError do |e, message|
      t = Template.new '{{#>whats_going_on}}'
      assert_equal "Expected identifier", message
    end
  end

  def test_section_without_end
    assert_raise SyntaxError do |e, message|
      t = Template.new '{{#section}}bla'
      assert_equal "Expected section end", message
    end
  end

  def test_section_without_end_identifier
    assert_raise SyntaxError do |e, message|
      t = Template.new '{{#section}}bla{{/}}'
      assert_equal "Expected identifier", message
    end
  end

  def test_partial_without_identifier
    assert_raise SyntaxError do |e, message|
      t = Template.new '{{>}}'
      assert_equal "Expected identifier", message
    end
  end

end
