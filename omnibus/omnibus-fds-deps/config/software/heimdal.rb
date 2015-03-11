name "heimdal"
default_version "1.6~git20131207+dfsg"

dependency "ncurses"

version "1.6~git20131207+dfsg" do
  source md5: "2797c863698845630f9d03781ab906e3"
end

source url: "http://archive.ubuntu.com/ubuntu/pool/main/h/heimdal/heimdal_#{version}.orig.tar.gz"

relative_path "heimdal-#{version}"

build do
  env = with_standard_compiler_flags(with_embedded_path)

  command "./autogen.sh", env: env
  command "./configure" \
          " --prefix=#{install_dir}/embedded", env: env #env: { 'LD_LIBRARY_PATH' => "#{install_dir}/embedded/lib" }

  make "-j #{workers}", env: env
#  make "install", env: env
end
