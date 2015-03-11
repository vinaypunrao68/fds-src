name "pcre"
default_version "8.31"

version "8.31" do
  source md5: "fab1bb3b91a4c35398263a5c1e0858c1"
end

source url: "http://archive.ubuntu.com/ubuntu/pool/main/p/pcre3/pcre3_#{version}.orig.tar.gz"

relative_path "pcre-#{version}"

build do
  env = with_standard_compiler_flags(with_embedded_path)

  command "./configure" \
          " --prefix=#{install_dir}/embedded", env: env

  make "-j #{workers}", env: env
  make "install", env: env

  # It's painful to figure out how to compile pcre3_8.31-2ubuntu2.debian.tar.gz to get the actual
  #  libpcre.so.3 library linked in. Much easier to add libpcre3 to devsetup and copy it from the
  #  packaging host
  copy "/lib/x86_64-linux-gnu/libpcre.so.3.13.1", "#{install_dir}/embedded/lib/libpcre.so.3.13.1"
  command "cd #{install_dir}/embedded/lib && ln -s libpcre.so.3.13.1 libpcre.so.3"
end
