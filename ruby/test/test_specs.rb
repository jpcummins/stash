require 'test/unit'
require 'yaml'
require 'lib/stash'

class SpecTest < Test::Unit::TestCase

  def spec_helper(file)
    YAML.load(File.read("test/specs/#{file}"))['tests'].each do |test|
      partials = {}
      if test['partials']
        test['partials'].each do |key, value|
          partials[key.to_sym] = Template.new value
        end
      end

      if test['data'] and test['data']['lambda']
        code = test['data']['lambda'].value['ruby']
        test['data']['lambda'] = lambda { eval "#{code}" }
      end

      test[:data] = test['data'].inject({}){|memo,(k,v)| memo[k.to_sym] = v; memo}

      yield test['name'], test['template'], test[:data], partials, test['expected']
    end
  end

  def test_comments
    spec_helper("comments.yml") do |test, content, data, partials, expected|
      t = Template.new content
      assert_equal expected, t.render(data, partials), test
    end
  end

  # def test_delimiters
  #   spec_helper("delimiters.yml") do |test, content, data, partials, expected|
  #     puts test
  #     t = Template.new content
  #     assert_equal expected, t.render(data, partials), test
  #   end
  # end

  def test_interpolation
    spec_helper("interpolation.yml") do |test, content, data, partials, expected|
      t = Template.new content
      assert_equal expected, t.render(data, partials), test
    end
  end

  def test_inverted
    spec_helper("inverted.yml") do |test, content, data, partials, expected|
      t = Template.new content
      assert_equal expected, t.render(data, partials), test
    end
  end

  def test_partials
    spec_helper("partials.yml") do |test, content, data, partials, expected|
      t = Template.new content
      assert_equal expected, t.render(data, partials), test
    end
  end

  def test_sections
    spec_helper("sections.yml") do |test, content, data, partials, expected|
      t = Template.new content
      assert_equal expected, t.render(data, partials), test
    end
  end

  def test_lambdas
    spec_helper("~lambdas.yml") do |test, content, data, partials, expected|
      t = Template.new content
      assert_equal expected, t.render(data, partials), test
    end
  end

end
