name "iscsi-scst"
default_version "3.0.0~rc1+svn5930"

source url: "http://ppa.launchpad.net/scst/3.0.x/ubuntu/pool/main/s/scst/#{name}_#{default_version}-ppa1~trusty_amd64.deb",
       md5: "c216d8844e062fd122773b53a184f946"

relative_path "iscsi-scst"

build do
  dest_dir = "#{install_dir}/embedded/"
  env = with_standard_compiler_flags(with_embedded_path)

  dest_dir = "#{install_dir}/embedded/"
  usr_dir = "usr/"
  sbin_dir = "#{usr_dir}sbin/"

  # Extract the code
  command "dpkg-deb -x *.deb ./", env:env

  mkdir "#{dest_dir}#{sbin_dir}"

  # Copy to distribution
  command "install #{sbin_dir}* #{dest_dir}#{sbin_dir}"
end
