name "libselinux"
default_version "2.2.2"

version "2.2.2" do
  source md5: "55026eb4654c4f732a27c191b39bebaf"
end

source url: "http://archive.ubuntu.com/ubuntu/pool/main/libs/libselinux/libselinux_#{version}.orig.tar.gz"

relative_path "libselinux-#{version}"

build do
  env = with_standard_compiler_flags(with_embedded_path)

  make "-j #{workers}", env: env
  make "install", env: {'PREFIX' => "#{install_dir}/embedded", 'DESTDIR' => "#{install_dir}/embedded" }
end
