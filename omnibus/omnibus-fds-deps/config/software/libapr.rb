name "libapr"
default_version "1.5.0"

version "1.5.0" do
  source md5: "cc93bd2c12d0d037f68e21cc6385dc31"
end

source url: "http://archive.ubuntu.com/ubuntu/pool/main/a/apr/apr_#{version}.orig.tar.bz2"

relative_path "apr-#{version}"

build do
  env = with_standard_compiler_flags(with_embedded_path)

  command "./configure" \
          " --prefix=#{install_dir}/embedded", env: env

  make "-j #{workers}", env: env
  make "install", env: env
end
