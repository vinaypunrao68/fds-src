name "badger"
default_version "0.1.3"

version "0.1.2" do
  source md5: "3b1fccfe84ff236df043b393918331c0"
end

version "0.1.3" do
  source md5: "7cfe32d0a4af0f3f18a04969141d97d0"
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
