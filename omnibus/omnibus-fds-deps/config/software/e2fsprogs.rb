name "e2fsprogs"
default_version "1.42.9"

version "1.42.9" do
  source md5: "3f8e41e63b432ba114b33f58674563f7"
end

source url: "http://archive.ubuntu.com/ubuntu/pool/main/e/e2fsprogs/e2fsprogs_#{version}.orig.tar.gz"

relative_path "e2fsprogs-#{version}"

build do
  env = with_standard_compiler_flags(with_embedded_path)

  command "mkdir bld"
  command "cd bld && ../configure" \
          " --prefix=#{install_dir}/embedded", env: env

  command "cd bld && make -j #{workers}", env: {'CFLAGS' => "-I#{install_dir}/embedded/include -fPID -shared", 'CXXFLAGS' => "-I#{install_dir}/embedded/include -fPID -shared"}
  command "cd bld && make install-libs", env: env
end
