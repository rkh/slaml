require 'minitest/unit'
require 'slaml/indent_parser'
require 'pp'

MiniTest::Unit.autorun if $0 == __FILE__

class TestIndentParser < MiniTest::Unit::TestCase  
  def self.parser(file = nil, &block)
    @count ||= 0
    @from    = file ? File.read(file) : ""
    @expect  = []
    @options = {}
    yield
    from, expect, options = @from, @expect, @options
    define_method("test_#{@count}") do
      from.gsub!(/^#{from[/^ */]}/, '')
      result = Slaml::IndentParser.new(options).compile(from)
      msg = "Expected:\n" << expect.pretty_inspect.gsub(/^/, "\t") << "\n"
      msg << "Was:\n" << result.pretty_inspect.gsub(/^/, "\t")
      assert_equal expect, result, msg
    end
    @count += 1
  end

  def self.from(value = nil)    @from    = value ? value : @from    end
  def self.expect(value = nil)  @expect  = value ? value : @expect  end
  def self.options(value = nil) @options = value ? value : @options end

  parser do
    from ''
    expect [:multi]
  end

  parser do
    from 'foo'
    expect [:multi, [:indent, 0, 'foo', [:multi]]]
  end

  parser do
    from "foo\n"
    expect [:multi, [:indent, 0, 'foo', [:multi]]]
  end

  parser do
    from <<-CODE
      foo
      bar
    CODE
    expect [:multi,
      [:indent, 0, 'foo', [:multi]],
      [:indent, 0, 'bar', [:multi]]]
  end

  parser do
    # stripping
    from <<-CODE
      foo 
      bar    
    CODE
    expect [:multi,
      [:indent, 0, 'foo', [:multi]],
      [:indent, 0, 'bar', [:multi]]]
  end

  parser do
    from <<-CODE
      foo
        bar
    CODE
    expect [:multi,
      [:indent, 0, 'foo', [:multi,
        [:indent, 2, 'bar', [:multi]]]]]
  end

  parser do
    from <<-CODE
      foo
        bar
        blah
    CODE
    expect [:multi,
      [:indent, 0, 'foo', [:multi,
        [:indent, 2, 'bar', [:multi]],
        [:indent, 2, 'blah', [:multi]]]]]
  end

  parser do
    from <<-CODE
      foo
        bar  
        blah
    CODE
    expect [:multi,
      [:indent, 0, 'foo', [:multi,
        [:indent, 2, 'bar', [:multi]],
        [:indent, 2, 'blah', [:multi]]]]]
      end

  parser do
    from <<-CODE
      foo
        bar
          blah
    CODE
    expect [:multi,
      [:indent, 0, 'foo', [:multi,
        [:indent, 2, 'bar', [:multi,
          [:indent, 4, 'blah', [:multi]]]]]]]
  end

  parser do
    from <<-CODE
      a
        b
      c
    CODE
    expect [:multi,
      [:indent, 0, 'a', [:multi,
        [:indent, 2, 'b', [:multi]]]],
        [:indent, 0, 'c', [:multi]]]
  end

  parser do
    from <<-CODE
      a
        b
        c
          d
    CODE
    expect [:multi,
      [:indent, 0, 'a', [:multi,
        [:indent, 2, 'b', [:multi]],
        [:indent, 2, 'c', [:multi,
          [:indent, 4, 'd', [:multi]]]]]]]
  end

  parser do
    from <<-CODE
      a  
        aa
      b 
      c
       ca
           caa
          cab 
      d
      e
    CODE
    expect [:multi,
      [:indent, 0, 'a', [:multi,
        [:indent, 2, 'aa', [:multi]]]],
        [:indent, 0, 'b', [:multi]],
        [:indent, 0, 'c', [:multi,
          [:indent, 1, 'ca', [:multi,
            [:indent, 5, 'caa', [:multi]],
            [:indent, 4, 'cab', [:multi]]]]]],
            [:indent, 0, 'd', [:multi]],
            [:indent, 0, 'e', [:multi]]]
  end

  parser do
    options :strip => false
    from "foo \n"
    expect [:multi, [:indent, 0, "foo ", [:multi]]]
  end

  parser do
    options :strip => false
    from "foo \n  bar"
    expect [:multi, [:indent, 0, "foo ", [:multi,
      [:indent, 2, "bar", [:multi]]]]]
  end

  parser do
    from <<-CODE
      a
      
      b
    CODE
    expect [:multi,
      [:indent, 0, 'a', [:multi]],
      [:indent, 0, '',  [:multi]],
      [:indent, 0, 'b', [:multi]]]
  end
  
  parser do
    from <<-CODE
      a
      
        b
    CODE
    expect [:multi,
      [:indent, 0, 'a', [:multi,
        [:indent, 2, '', [:multi]],
        [:indent, 2, 'b', [:multi]]]]]
  end

  parser do
    options :track_empty_lines => false
    from <<-CODE
      a
      
      b
    CODE
    expect [:multi,
      [:indent, 0, 'a', [:multi]],
      [:indent, 0, 'b', [:multi]]]
  end
  
  parser do
    options :track_empty_lines => false
    from <<-CODE
      a
      
        b
    CODE
    expect [:multi,
      [:indent, 0, 'a', [:multi,
        [:indent, 2, 'b', [:multi]]]]]
  end
end
