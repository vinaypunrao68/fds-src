name "dkms"
default_version "2.2.0.3-2"

source git: "git://anonscm.debian.org/collab-maint/dkms.git"

relative_path "dkms"

build do
  debian_dir = "debian/"
  patch_dir = "#{debian_dir}/patches/"
  script_dir = "#{debian_dir}/scripts/"

  dest_dir = "#{install_dir}/embedded/"
  usr_dir = "#{dest_dir}usr/"
  bin_dir = "#{usr_dir}bin/"
  share_dir = "#{usr_dir}share/"
  debhelper_dir = "#{share_dir}debhelper/autoscripts/"
  sequence_dir = "#{share_dir}perl5/Debian/Debhelper/Sequence/"
  env = with_standard_compiler_flags(with_embedded_path)

  # Patch the code
  command "git checkout -f debian/#{default_version}", env:env
  command "for i in $(cat #{patch_dir}series); do patch -Np1 < #{patch_dir}$i; done", env:env

  # Customize configuration
  command "echo -n 'source_tree=\"/opt/fds-deps/embedded/usr/src\"\ndkms_tree=\"/opt/fds-deps/embedded/var/lib/dkms\"' >> dkms_framework.conf", env:env

  mkdir bin_dir
  mkdir debhelper_dir
  mkdir sequence_dir

  # Build DKMS
  command "make install-debian", env:env.merge({"DESTDIR" => dest_dir})

  # Copy debian helper scripts
  command "install #{script_dir}/dh_dkms #{bin_dir}"
  command "install #{script_dir}/*-dkms #{debhelper_dir}"
  command "install #{script_dir}/dkms.pm #{sequence_dir}"
end
