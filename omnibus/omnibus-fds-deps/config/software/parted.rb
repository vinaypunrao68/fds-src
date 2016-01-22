name "parted"
default_version "2.3"

dependency "libiconv"
dependency "readline"
dependency "libselinux"
dependency "lvm"
dependency "util-linux"
dependency "pcre"
dependency "ncurses"

version "2.3" do
  source md5: "01d93eaaa3f290a17dd9d5dbfc7bb927"
end

version "3.2" do
  source md5: "0247b6a7b314f8edeb618159fa95f9cb"
end

source url: "http://ftp.gnu.org/gnu/parted/parted-#{version}.tar.xz"

relative_path "parted-#{version}"

build do
  env = with_standard_compiler_flags(with_embedded_path)
  env['CFLAGS'] << " -liconv"

  command "./configure" \
          " --prefix=#{install_dir}/embedded", env: env

  command "sed -i -e '/gets is a security/d' lib/stdio.in.h"

  make "-j #{workers}", env: env
  make "install", env: env
end
