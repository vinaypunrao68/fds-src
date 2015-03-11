name "sshpass"
default_version "1.05"

version("1.05") { source md5: "c52d65fdee0712af6f77eb2b60974ac7" }

source url: "http://archive.ubuntu.com/ubuntu/pool/universe/s/sshpass/sshpass_#{version}.orig.tar.gz"

relative_path "sshpass-#{version}"

build do
  env = with_standard_compiler_flags(with_embedded_path)

  env["CFLAGS"] << " -DNO_VIZ" if solaris?

  command "./configure" \
          " --prefix=#{install_dir}/embedded", env: env

  command "make", env: env
  command "make install", env: env
end
