name "readline"
default_version "5.2"

version "5.2" do
  source md5: "cb4f190fdffad9e38428df9962cd4f64"
end

source url: "http://archive.ubuntu.com/ubuntu/pool/main/r/readline5/readline5_#{version}+dfsg.orig.tar.gz"

relative_path "readline5-#{version}+dfsg"

build do
  env = with_standard_compiler_flags(with_embedded_path)

  command "./configure" \
          " --prefix=#{install_dir}/embedded", env: env

  make "-j #{workers}", env: env
  make "install", env: env
end
