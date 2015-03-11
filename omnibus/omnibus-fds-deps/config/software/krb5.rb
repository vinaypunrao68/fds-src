name "krb5"
default_version "1.12+dfsg"

version "1.12+dfsg" do
  source md5: "04751d9c356c150d4f8ba7d5b8f64686"
end

source url: "http://archive.ubuntu.com/ubuntu/pool/main/k/krb5/krb5_#{version}.orig.tar.gz"

relative_path "krb5-#{version}"

build do
  env = with_standard_compiler_flags(with_embedded_path)

  command "cd src && ./configure" \
          " --prefix=#{install_dir}/embedded", env: env

  command "cd src && make -j #{workers}", env: env
  command "cd src && make install", env: env
end
