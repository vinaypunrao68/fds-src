name "openldap"
default_version "2.4.31"

version "2.4.31" do
  source md5: "f5ae45ed6e86debb721b68392b5ce13c"
end

source url: "http://archive.ubuntu.com/ubuntu/pool/main/o/openldap/openldap_#{version}.orig.tar.gz"

relative_path "openldap-#{version}"

build do
  env = with_standard_compiler_flags(with_embedded_path)

  command "./configure" \
          " --prefix=#{install_dir}/embedded", env: env

  make "-j #{workers}", env: env
  make "install", env: env
end
