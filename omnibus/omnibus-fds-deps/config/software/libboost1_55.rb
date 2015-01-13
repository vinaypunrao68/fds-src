name "libboost1_55"
default_version "1.55"

source url: "http://archive.ubuntu.com/ubuntu/pool/main/b/boost1.55/boost1.55_1.55.0.orig.tar.bz2"
source md5: "d6eef4b4cacb2183f2bf265a5a03a354"

relative_path "boost_1_55_0"

build do
  env = with_standard_compiler_flags(with_embedded_path)

  command "./bootstrap.sh --with-libraries=log,program_options,timer,thread,regex,date_time,system,chrono --prefix=#{install_dir}/embedded --libdir=#{install_dir}/embedded/lib --includedir=#{install_dir}/embedded/include --exec-prefix=#{install_dir}/embedded", env: env
  command "export LD_RUN_PATH=#{install_dir}/embedded/lib && ./b2 -j #{workers} install --prefix=#{install_dir}/embedded"

end
