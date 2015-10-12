name "scst-dkms"
default_version "3.0.0~rc1+svn5930"

source url: "http://ppa.launchpad.net/scst/3.0.x/ubuntu/pool/main/s/scst/#{name}_#{default_version}-ppa1~trusty_amd64.deb",
       md5: "b6400bcfcf7be6866a6e555e2d06e048"

relative_path "scst"

build do
  dest_dir = "#{install_dir}/embedded/"
  env = with_standard_compiler_flags(with_embedded_path)

  dest_dir = "#{install_dir}/embedded/"
  usr_dir = "#{dest_dir}usr/"

  # Extract the code
  command "dpkg-deb -x *.deb ./", env:env

  # Extract the code
  patch source: "perfrelease.diff", plevel: 1

  # Copy to distribution
  command "cp -av usr/src #{usr_dir}", env:env
end
