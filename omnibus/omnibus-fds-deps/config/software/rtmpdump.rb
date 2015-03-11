name "rtmpdump"
default_version "2.4+20121230.gitdf6c518"

dependency "openssl"

source url: "http://archive.ubuntu.com/ubuntu/pool/main/r/rtmpdump/rtmpdump_#{version}.orig.tar.gz",
  md5: "8c996f6ceade12da869ca95d1aa75ecb"

relative_path "rtmpdump-20121230"

build do
  env = with_standard_compiler_flags(with_embedded_path)

  make "prefix=#{install_dir}/embedded -j #{workers}", env: env
  make "prefix=#{install_dir}/embedded install", env: env
end
