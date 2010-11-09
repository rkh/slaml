namespace :build do
  desc "compile slim/indent_parser"
  task :indent_parser do
    files = nil
    cd 'ext/slaml/indent_parser' do
      ruby "extconf.rb"
      sh "make"
      files = Dir["indent_parser.*"] - ["indent_parser.c", "indent_parser.o"]
      rm_rf "indent_parser.o"
      rm_rf "Makefile"
    end
    mkdir_p "lib/slaml"
    files.each { |f| mv "ext/slaml/indent_parser/#{f}", "lib/slaml/#{f}" }
  end
end

desc "run specs"
task :spec do
  $LOAD_PATH.unshift 'spec', 'lib'
  require 'minitest/unit'
  MiniTest::Unit.autorun
  Dir.glob('test/**/*_test.rb') do |file|    
    load file
  end
end

namespace :defs do
  task :indent_parser do
    File.read('ext/slaml/indent_parser/indent_parser.c').scan /^((static|void).*)$/ do |line, *_|
      puts line << ";"
    end
  end
end

desc "compile everything"
task :build   => "build:indent_parser"
task :test    => :spec
task :default => [:build, :spec]
