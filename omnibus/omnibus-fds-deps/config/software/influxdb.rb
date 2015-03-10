name "influxdb"
default_version "0.8.8"

version "0.8.8" do
  source md5: "2f4693ecbe618340e92d10e4903fe647"
end

source url: "https://s3.amazonaws.com/influxdb/influxdb-#{version}.amd64.tar.gz"

# whitelist is required since we're not building influxdb from source, 
#  causing Omnibus to complain since it's linked against system versions
#  of libz.so and libbz2.so. Instead we'll force Influx to use 
#  /opt/fds-deps/embedded/lib via Upstart config
whitelist_file "influxdb"

relative_path "build"

build do
  env = with_standard_compiler_flags(with_embedded_path)

  mkdir "#{install_dir}/embedded/influxdb"
  sync "#{project_dir}/", "#{install_dir}/embedded/influxdb"
end
