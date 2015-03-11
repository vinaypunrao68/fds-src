name "libevent"
default_version "2.0.21"

version "2.0.21" do
  source md5: "b2405cc9ebf264aa47ff615d9de527a2"
end

source url: "http://archive.ubuntu.com/ubuntu/pool/main/libe/libevent/libevent_#{version}-stable.orig.tar.gz"

relative_path "libevent-#{version}-stable"

build do
  env = with_standard_compiler_flags(with_embedded_path)

  command "./configure" \
          " --prefix=#{install_dir}/embedded", env: env

  make "-j #{workers}", env: env
  make "install", env: env
end
