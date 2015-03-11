name "google-perftools"
default_version "2.1"

version "2.1" do
  source md5: "5e5a981caf9baa9b4afe90a82dcf9882"
end

source url: "http://archive.ubuntu.com/ubuntu/pool/main/g/google-perftools/google-perftools_#{version}.orig.tar.gz"

relative_path "gperftools-#{version}"

build do
  env = with_standard_compiler_flags(with_embedded_path)

  command "./configure" \
          " --prefix=#{install_dir}/embedded", env: env

  make "-j #{workers}", env: env
  make "install", env: env
end
