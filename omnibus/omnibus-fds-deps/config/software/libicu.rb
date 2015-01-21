name "libicu"
default_version "52.1"

version "52.1" do
  source md5: "9e96ed4c1d99c0d14ac03c140f9f346c"
end

source url: "http://archive.ubuntu.com/ubuntu/pool/main/i/icu/icu_#{version}.orig.tar.gz"

relative_path "icu"

build do
  env = with_standard_compiler_flags(with_embedded_path)

  command "cd source && ./configure" \
          " --prefix=#{install_dir}/embedded", env: env

  command "cd source && make -j #{workers}", env: env
  command "cd source && make install", env: env
end
