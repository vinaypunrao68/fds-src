name "libidn"
default_version "1.28"

version "1.28" do
  source md5: "43a6f14b16559e10a492acc65c4b0acc"
end

source url: "http://archive.ubuntu.com/ubuntu/pool/main/libi/libidn/libidn_#{version}.orig.tar.gz"

relative_path "libidn-#{version}"

build do
  env = with_standard_compiler_flags(with_embedded_path)

  command "./configure" \
          " --prefix=#{install_dir}/embedded", env: env

  make "-j #{workers}", env: env
  make "install", env: env
end
