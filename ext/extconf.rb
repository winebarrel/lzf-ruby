require 'mkmf'

if have_library('lzf')
  create_makefile('lzfruby')
end
