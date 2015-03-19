name "libgcrypt"
default_version "1.6.3"

version "1.6.3" do
  source md5: "4262c3aadf837500756c2051a5c4ae5e"
end

dependency "libgpg-error"

source url: "ftp://ftp.gnupg.org/gcrypt/libgcrypt/libgcrypt-1.6.3.tar.bz2"

relative_path "libgcrypt-#{version}"

build do
  env = with_standard_compiler_flags(with_embedded_path)

  command "./configure" \
          " --prefix=#{install_dir}/embedded", env: env

  make "-j #{workers}", env: env
  make "install", env: env

end
