name "libcryptopp"
default_version "562"

source url: "https://github.com/cboggs/cryptopp562/raw/master/cryptopp#{version}.tar.gz",
    md5: "fc3064d421f015d929c43005b1e14f8e"

relative_path "cryptopp#{version}"

build do
  env = with_standard_compiler_flags(with_embedded_path)
  env['CFLAGS'] << " -x c++ -O2 -march=native -pipe -c -I headers blowfish.cpp"
  env['CXXFLAGS'] << " -x c++ -O2 -march=native -pipe -c -I headers blowfish.cpp"
  command "sed -i -e 's/# CXXFLAGS += -fPIC/CXXFLAGS += -fPIC/' GNUmakefile"
  make "-j #{workers}", env: env
  make "libcryptopp.so -j #{workers}", env: env
  make "install PREFIX=#{install_dir}/embedded", env: env
end
