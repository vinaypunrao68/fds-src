name "libxml2"
default_version "2.9.1"

version "2.9.1" do
  source md5: "5f111980c06f927a62492b7b9781b7bf"
end

source url: "http://archive.ubuntu.com/ubuntu/pool/main/libx/libxml2/libxml2_#{version}+dfsg1.orig.tar.gz"

relative_path "libxml2-#{version}+dfsg1"

build do
  env = with_standard_compiler_flags(with_embedded_path)

  command "./configure --prefix=#{install_dir}/embedded", env:env
  make "-j #{workers}", env:env

  sync "#{project_dir}/include/libxml", "#{install_dir}/embedded/include/libxml"
  copy "#{project_dir}/.libs/libxml2.*", "#{install_dir}/embedded/lib/"
end
