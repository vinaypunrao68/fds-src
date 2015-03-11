name "libical"
default_version "1.0.1"

version "1.0.1" do
  source md5: "d26a58149aaae9915fa79fb35c8de6fc"
end

source url: "https://github.com/libical/libical/releases/download/v#{version}/libical-#{version}.tar.gz"

relative_path "libical-#{version}"

build do
  env = with_standard_compiler_flags(with_embedded_path)

  command "mkdir -p bld"
  command "cd bld && cmake -DCMAKE_INSTALL_PREFIX=#{install_dir}/embedded -DSHARED_ONLY=YES ..", env: env
  command "sed -i '68,69d' bld/src/libicalss/cmake_install.cmake"
  command "sed -i '68,69d' bld/src/libicalvcal/cmake_install.cmake"
  command "cd bld && make -j #{workers}", env: env
  command "cd bld && make install", env: env

end
