name "gmond-influxdb-bridge"
default_version "0.2.2"

dependency "python-influxdb"
dependency "python-lxml"

version "0.2.2" do
  source md5: "d8bd24add2ed1b9a15238457971881c2"
end

source url: "https://github.com/cboggs/gmond-influxdb-bridge/archive/#{version}.tar.gz"

relative_path "gmond-influxdb-bridge-#{version}"

fds_src_dir = ENV['FDS_SRC_DIR']
raise "FDS_SRC_DIR environment variable must be set" unless fds_src_dir

build do
  mkdir "#{install_dir}/embedded/etc/init"
  copy "#{project_dir}/gmond-influxdb-bridge.py", "#{install_dir}/embedded/bin/gmond-influxdb-bridge.py"
  copy "#{fds_src_dir}/ansible/files/gmond/gmond-influxdb-bridge.json", "#{install_dir}/embedded/etc/gmond-influxdb-bridge.json"
  copy "#{fds_src_dir}/ansible/files/gmond/upstart_gmond-influxdb-bridge.conf", "#{install_dir}/embedded/etc/init/gmond-influxdb-bridge.conf"
end
