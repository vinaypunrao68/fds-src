name "badger"
default_version "master"

source git: "git://github.com/cboggs/stat-badger"

dependency "python-thrift"

# whitelist is required since we're not building influxdb from source, 
#  causing Omnibus to complain since it's linked against system versions
#  of libz.so and libbz2.so. Instead we'll force Influx to use 
#  /opt/fds-deps/embedded/lib via Upstart config

relative_path "stat-badger"

build do
  env = with_standard_compiler_flags(with_embedded_path)

  mkdir "#{install_dir}/embedded/stat-badger"
  sync "#{project_dir}/", "#{install_dir}/embedded/stat-badger"

end
