name "mdadm"
default_version "3.2.5"

dependency "binutils"

version "3.2.5" do
  source md5: "83ba4a6249ae24677e915e44c9cfcc58"
end

source url: "http://archive.ubuntu.com/ubuntu/pool/main/m/mdadm/mdadm_#{version}.orig.tar.bz2"

relative_path "mdadm-#{version}"

build do
  env = with_standard_compiler_flags(with_embedded_path)

  make "-j #{workers}", env: env
  command "make DESTDIR=#{install_dir}/embedded install", env: env

end
