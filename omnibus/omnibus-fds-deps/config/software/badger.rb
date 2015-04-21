name "badger"
default_version "0.1.0"

version "0.1.0" do
  source md5: "db0c3ac0a764f61716d8e79a8cc3ee51"
end

source url: "http://bld-dev-02/stat-badger-#{version}.tar.gz"

# whitelist is required since we're not building influxdb from source, 
#  causing Omnibus to complain since it's linked against system versions
#  of libz.so and libbz2.so. Instead we'll force Influx to use 
#  /opt/fds-deps/embedded/lib via Upstart config

relative_path "stat-badger-#{version}"

build do
  env = with_standard_compiler_flags(with_embedded_path)

  mkdir "#{install_dir}/embedded/stat-badger"
  sync "#{project_dir}/", "#{install_dir}/embedded/stat-badger"
end
