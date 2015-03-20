name "libconfuse"
default_version "2.7"

version "2.7" do
  source md5: "45932fdeeccbb9ef4228f1c1a25e9c8f"
end

source url: "http://savannah.nongnu.org/download/confuse/confuse-#{version}.tar.gz"

relative_path "confuse-#{version}"

build do
  env = with_standard_compiler_flags(with_embedded_path)

  command "./configure" \
          " --prefix=#{install_dir}/embedded --enable-shared", env: env

  make "-j #{workers}", env: env
  make "install", env: env
end
