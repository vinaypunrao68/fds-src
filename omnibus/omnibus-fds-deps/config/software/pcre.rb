name "pcre"
default_version "8.31"

version "8.31" do
  source md5: "fab1bb3b91a4c35398263a5c1e0858c1"
end

source url: "http://archive.ubuntu.com/ubuntu/pool/main/p/pcre3/pcre3_#{version}.orig.tar.gz"

relative_path "pcre-#{version}"

build do
  env = with_standard_compiler_flags(with_embedded_path)

  command "./configure" \
          " --prefix=#{install_dir}/embedded --enable-cpp --enable-utf8 --enable-unicode-properties", env: env

  make "-j #{workers}", env: env
  make "install", env: env
end
