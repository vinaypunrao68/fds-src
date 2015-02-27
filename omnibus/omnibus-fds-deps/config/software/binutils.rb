name "binutils"
default_version "2.24"

version "2.24" do
  source md5: "a5dd5dd2d212a282cc1d4a84633e0d88"
end

source url: "http://archive.ubuntu.com/ubuntu/pool/main/b/binutils/binutils_#{version}.orig.tar.gz"

relative_path "binutils-#{version}"

build do
  env = with_standard_compiler_flags(with_embedded_path)

  command "./configure" \
          " --prefix=#{install_dir}/embedded --with-sysroot=#{install_dir}/embedded", env: env

  make "-j #{workers}", env: env
  make "install", env: env
end
