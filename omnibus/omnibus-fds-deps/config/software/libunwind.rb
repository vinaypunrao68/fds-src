name "libunwind"
default_version "1.1"

version "1.1" do
  source md5: "fb4ea2f6fbbe45bf032cd36e586883ce"
end

source url: "http://archive.ubuntu.com/ubuntu/pool/main/libu/libunwind/libunwind_#{version}.orig.tar.gz"

relative_path "libunwind-#{version}"

build do
  env = with_standard_compiler_flags(with_embedded_path)

  command "./configure" \
          " --prefix=#{install_dir}/embedded", env: env

  make "-j #{workers}", env: env
  make "install", env: env
end
