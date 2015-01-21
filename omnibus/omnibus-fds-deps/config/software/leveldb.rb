name "leveldb"
default_version "1.15.0"

version "1.15.0" do
  source md5: "e91fd7cbced8b84e21f357a866ad226a"
end

source url: "http://archive.ubuntu.com/ubuntu/pool/main/l/leveldb/leveldb_1.15.0.orig.tar.gz"

relative_path "leveldb-#{version}"

build do
  env = with_standard_compiler_flags(with_embedded_path)

  make "-j #{workers}", env: env
  command "cp -av libleveldb.* #{install_dir}/embedded/lib/", env: env
end
