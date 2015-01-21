name "libical"
default_version "1.0"

version "1.0" do
  source md5: "4438c31d00ec434f02867a267a92f8a1"
end

source url: "http://archive.ubuntu.com/ubuntu/pool/main/libi/libical/libical_#{version}.orig.tar.gz"

relative_path "libical-#{version}"

build do
  env = with_standard_compiler_flags(with_embedded_path)

  command "./bootstrap"
  command "./configure" \
        " --prefix=#{install_dir}/embedded", env: env
  make "-j #{workers}", env: env
  make "install", env: env
end
