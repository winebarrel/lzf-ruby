Gem::Specification.new do |spec|
  spec.name              = 'lzfruby'
  spec.version           = '0.1.3'
  spec.summary           = 'Ruby bindings for LibLZF.'
  spec.files             = Dir.glob('ext/**/*.*') + %w(ext/extconf.rb README)
  spec.author            = 'winebarrel'
  spec.email             = 'sgwr_dts@yahoo.co.jp'
  spec.homepage          = 'https://bitbucket.org/winebarrel/lzf-ruby'
  spec.extensions        = 'ext/extconf.rb'
  spec.has_rdoc          = true
  spec.rdoc_options      << '--title' << 'LZF/Ruby - Ruby bindings for LibLZF.'
  spec.extra_rdoc_files  = %w(README ext/lzfruby.c)
end
