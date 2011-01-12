Gem::Specification.new do |spec|
  spec.name              = 'lzfruby'
  spec.version           = '0.1.3'
  spec.platform          = 'mswin32'
  spec.summary           = 'Ruby bindings for LibLZF.'
  spec.require_paths     = %w(lib/i386-mswin32)
  spec.files             = %w(lib/i386-mswin32/lzfruby.so README ext/lzfruby.c)
  spec.author            = 'winebarrel'
  spec.email             = 'sgwr_dts@yahoo.co.jp'
  spec.homepage          = 'https://bitbucket.org/winebarrel/lzf-ruby'
  spec.has_rdoc          = true
  spec.rdoc_options      << '--title' << 'LZF/Ruby - Ruby bindings for LibLZF.'
  spec.extra_rdoc_files  = %w(README ext/lzfruby.c)
end
