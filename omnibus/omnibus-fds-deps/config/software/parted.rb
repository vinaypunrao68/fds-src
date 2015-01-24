name "parted"
default_version "3.2"

dependency "libiconv"
dependency "readline"
dependency "libselinux"
dependency "lvm"
dependency "util-linux"
dependency "pcre"
dependency "ncurses"

version "3.2" do
  source md5: "0247b6a7b314f8edeb618159fa95f9cb"
end

source url: "http://gnu.mirrorcatalogs.com/parted/parted-#{version}.tar.xz"

relative_path "parted-#{version}"

build do
  env = with_standard_compiler_flags(with_embedded_path)
  env['CFLAGS'] << " -liconv"

  command "./configure" \
          " --prefix=#{install_dir}/embedded", env: env

  make "-j #{workers}", env: env
#env: { 'CFLAGS' => "-I#{install_dir}/embedded/include -liconv", 'CXXFLAGS' => "-I#{install_dir}/embedded/include -liconv"}
  make "install", env: env
end
