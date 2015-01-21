name "xfsprogs"
default_version "3.1.9"

dependency "util-linux"

version "3.1.9" do
  source md5: "3bdced1840fe8c0053b076224125b2a2"
end

source url: "ftp://oss.sgi.com/projects/xfs/cmd_tars/xfsprogs-#{version}.tar.gz"

relative_path "xfsprogs-#{version}"

build do
  env = with_standard_compiler_flags(with_embedded_path)

  command "OPTIMIZER=-O1 DEBUG=-DNDEBUG LOCAL_CONFIGURE_OPTIONS='--prefix=#{install_dir}/embedded' make", env: env
  make "-j #{workers} install", env: env
end
