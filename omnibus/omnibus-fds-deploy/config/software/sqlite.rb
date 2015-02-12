name "sqlite"
default_version "3.8.2"

version "3.8.2" do
  source md5: "818a7530fa5dc6a0e3204101356a1bb5"
end

source url: "http://archive.ubuntu.com/ubuntu/pool/main/s/sqlite3/sqlite3_#{version}.orig.tar.gz"

relative_path "sqlite3-#{version}"

build do
  env = with_standard_compiler_flags(with_embedded_path)

  command "mkdir -p bld"
  command "cd bld && TCLSH_CMD=#{install_dir}/embedded/bin/tclsh8.6 ../configure" \
          " --prefix=#{install_dir}/embedded", env: env

  command "cd bld && make -j #{workers}"
  command "cd bld && make install"
end
