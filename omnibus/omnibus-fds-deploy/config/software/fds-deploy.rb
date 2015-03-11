
# These options are required for all software definitions
name "fds-deploy"

fds_version = "0.7.0"
build_type = ENV['BUILD_TYPE'] 
git_sha = `git rev-parse HEAD`.chomp
fds_src_dir = ENV['FDS_SRC_DIR']

raise "FDS_SRC_DIR must be set'" unless fds_src_dir
raise "You need to have a release build available locally at #{fds_src_dir}" unless !Dir.glob("#{fds_src_dir}/omnibus/omnibus-fds-platform/pkg/fds-platform*release*.deb").empty?
raise "You need to have a fds-deps package available locally at #{fds_src_dir}" unless !Dir.glob("#{fds_src_dir}/omnibus/omnibus-fds-deps/pkg/fds-deps*.deb").empty?

default_version '0.7.0'

build do
    mkdir "#{install_dir}/omnibus/omnibus-fds-platform/pkg"
    mkdir "#{install_dir}/omnibus/omnibus-fds-deps/pkg"
    mkdir "#{install_dir}/source/config/etc"
    mkdir "#{install_dir}/source/tools/install"
    mkdir "#{install_dir}/source/platform/python"
    mkdir "#{install_dir}/source/test"
    mkdir "#{install_dir}/ansible"
    sync "#{fds_src_dir}/omnibus/omnibus-fds-platform/pkg", "#{install_dir}/omnibus/omnibus-fds-platform/pkg"
    sync "#{fds_src_dir}/omnibus/omnibus-fds-deps/pkg", "#{install_dir}/omnibus/omnibus-fds-deps/pkg"
    sync "#{fds_src_dir}/ansible", "#{install_dir}/ansible", exclude: "**/resources"
    sync "#{fds_src_dir}/source/config/etc", "#{install_dir}/source/config/etc", exclude: "*.conf"
    copy "#{fds_src_dir}/source/tools/install/fdsinstall.py", "#{install_dir}/source/tools/install/fdsinstall.py"
    copy "#{fds_src_dir}/source/test/formation.conf.j2", "#{install_dir}/source/test/formation.conf.j2"
    copy "#{fds_src_dir}/source/platform/python/disk_type.py", "#{install_dir}/source/platform/python/disk_type.py"
    link "#{install_dir}/source/config/etc/fds_config_defaults.j2", "#{install_dir}/source/test/fds_config_defaults.j2"
end
