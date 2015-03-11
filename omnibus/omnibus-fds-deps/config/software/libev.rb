name "libev"
default_version "4.15"

version "4.15" do
  source md5: "3a73f247e790e2590c01f3492136ed31"
end

source url: "http://archive.ubuntu.com/ubuntu/pool/universe/libe/libev/libev_#{version}.orig.tar.gz"

relative_path "libev-#{version}"

build do
  env = with_standard_compiler_flags(with_embedded_path)

  command "./configure" \
          " --prefix=#{install_dir}/embedded", env: env

  make "-j #{workers}", env: env
  make "install", env: env
end
