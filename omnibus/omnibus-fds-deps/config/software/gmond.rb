name "gmond"
default_version "3.6.0"

dependency "libapr"
dependency "libconfuse"
dependency "expat"
dependency "gmond-influxdb-bridge"

version "3.6.0" do
  source md5: "05926bb18c22af508a3718a90b2e9a2c"
end

source url: "http://archive.ubuntu.com/ubuntu/pool/universe/g/ganglia/ganglia_#{version}.orig.tar.gz"

relative_path "ganglia-#{version}"

fds_src_dir = ENV['FDS_SRC_DIR']
raise "FDS_SRC_DIR environment variable must be set" unless fds_src_dir

build do
  env = with_standard_compiler_flags(with_embedded_path)

  command "./configure" \
          " --prefix=#{install_dir}/embedded", env: env

  make "-j #{workers}", env: env
  make "install", env: env

  mkdir "#{install_dir}/embedded/etc/init"
  copy "#{fds_src_dir}/ansible/files/gmond/upstart_gmond.conf", "#{install_dir}/embedded/etc/init/gmond.conf"
  copy "#{fds_src_dir}/ansible/files/gmond/base_gmond.conf", "#{install_dir}/embedded/etc/gmond.conf"

end
