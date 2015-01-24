name "libjemalloc"
default_version "3.6.0"

version "3.6.0" do
  source md5: "e76665b63a8fddf4c9f26d2fa67afdf2"
end

source url: "http://archive.ubuntu.com/ubuntu/pool/universe/j/jemalloc/jemalloc_#{version}.orig.tar.bz2"

relative_path "jemalloc-#{version}"

build do
  env = with_standard_compiler_flags(with_embedded_path)

  command "./configure" \
          " --prefix=#{install_dir}/embedded", env: env

  make "-j #{workers}", env: env
  make "install", env: env
end
