name "libhiredis"
default_version "0.11.0"

version "0.11.0" do
  source md5: "5a42727ea95d3a12a782433199a7c0ba"
  relative_path "antirez-hiredis-0fff0f1"
end

source url: "http://archive.ubuntu.com/ubuntu/pool/universe/h/hiredis/hiredis_#{version}.orig.tar.gz"

build do
  env = with_standard_compiler_flags(with_embedded_path)

  command "make -j #{workers} PREFIX=#{install_dir}/embedded", env: env
  command "make PREFIX=#{install_dir}/embedded install", env: env
end
