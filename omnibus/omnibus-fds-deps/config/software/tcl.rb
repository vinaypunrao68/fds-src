name "tcl"
default_version "8.6.3"

version "8.6.3" do
  source md5: "db382feca91754b7f93da16dc4cdad1f"
end

dependency "zlib"

# source url: "http://prdownloads.sourceforge.net/tcl/tcl#{version}-src.tar.gz"
source url: "https://s3-us-west-2.amazonaws.com/fds-tcl/tcl#{version}-src.tar.gz"

relative_path "tcl#{version}"

build do
  env = with_standard_compiler_flags(with_embedded_path)

  command "cd unix && ./configure" \
          " --prefix=#{install_dir}/embedded", env: env

  command "cd unix && make -j #{workers}", env:env
  command "cd unix && make install", env:env

end
