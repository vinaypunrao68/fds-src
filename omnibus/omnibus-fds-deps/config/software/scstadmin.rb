name "scstadmin"
default_version "3.0.0~rc1+svn5930"

source url: "http://ppa.launchpad.net/scst/3.0.x/ubuntu/pool/main/s/scst/#{name}_#{default_version}-ppa1~trusty_amd64.deb",
       md5: "702a8d01f0e5783f63bbd34f500ad778"

relative_path "scstadmin"

build do
  dest_dir = "#{install_dir}/embedded/"
  env = with_standard_compiler_flags(with_embedded_path)

  dest_dir = "#{install_dir}/embedded/"
  etc_dir = "etc/"
  svc_dir = "#{etc_dir}init.d/"
  usr_dir = "usr/"
  sbin_dir = "#{usr_dir}sbin/"
  share_dir = "#{usr_dir}share/"
  perl_dir = "#{share_dir}perl/5.18.2/SCST/"

  # Extract the code
  command "dpkg-deb -x *.deb ./", env:env

  patch source: "config.diff", plevel: 1
  patch source: "init.diff", plevel: 1

  mkdir "#{dest_dir}#{perl_dir}"
  mkdir "#{dest_dir}#{sbin_dir}"
  mkdir "#{dest_dir}#{svc_dir}"

  # Copy to distribution
  command "install #{perl_dir}* #{dest_dir}#{perl_dir}"
  command "install #{sbin_dir}* #{dest_dir}#{sbin_dir}"
  command "install #{etc_dir}scst.conf #{dest_dir}#{etc_dir}"
  command "install #{svc_dir}* #{dest_dir}#{svc_dir}"
end
