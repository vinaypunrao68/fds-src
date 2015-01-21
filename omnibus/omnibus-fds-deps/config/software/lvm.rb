name "lvm"
default_version "2.02.98"

version "2.02.98" do
  source md5: "f8ed4a225ae7c07f07f7081f2971ba1f"
end

source url: "http://archive.ubuntu.com/ubuntu/pool/main/l/lvm2/lvm2_#{version}.orig.tar.gz"

relative_path "lvm2-#{version}"

build do
  env = with_standard_compiler_flags(with_embedded_path)

  command "./configure" \
          " --prefix=#{install_dir}/embedded --with-lvm1=none", env: env

  make "device-mapper -j #{workers}", env: env
  make "install_device-mapper", env: env
end
