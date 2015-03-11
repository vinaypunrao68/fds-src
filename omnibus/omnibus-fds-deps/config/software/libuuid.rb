name "libuuid"
default_version "2.20.1"

version "2.20.1" do
  source md5: "00bbf6e9698a713a9452c91b76eda756"
end

source url: "http://archive.ubuntu.com/ubuntu/pool/main/u/util-linux/util-linux_#{version}.orig.tar.gz"

relative_path "util-linux-#{version}"

build do
  env = with_standard_compiler_flags(with_embedded_path)

  command "./configure" \
          " --prefix=#{install_dir}/embedded", env: env

  command "cd libuuid && make -j #{workers}", env: env
  command "cd libuuid && make install", env: env
end
