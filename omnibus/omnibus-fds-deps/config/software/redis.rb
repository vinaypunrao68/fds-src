name "redis"
default_version "2.8.19"

version "2.8.19" do
  source md5: "3794107224043465603f48941f5c86a7"
end

source url: "http://download.redis.io/releases/redis-#{version}.tar.gz"

relative_path "redis-#{version}"

build do
  env = with_standard_compiler_flags(with_embedded_path)

  make "-j #{workers}", env: env
  make "PREFIX=#{install_dir}/embedded install", env: env
end
