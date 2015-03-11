name "libconfig"
default_version "1.4.9"

version "1.4.9" do
  source md5: "b6ee0ce2b3ef844bad7cac2803a90634"
end

source url: "http://ftp.de.debian.org/debian/pool/main/libc/libconfig/libconfig_#{version}.orig.tar.gz"

relative_path "libconfig-#{version}"

build do
  env = with_standard_compiler_flags(with_embedded_path)

  command "./configure" \
          " --prefix=#{install_dir}/embedded", env: env

  make "-j #{workers}", env: env
  make "install", env: env
end
