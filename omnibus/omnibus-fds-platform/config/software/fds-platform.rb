# These options are required for all software definitions
name "fds-platform"

mydir = File.dirname(__FILE__)
fds_version = File.readlines("#{mydir}/../../../VERSION").first.chomp
build_type = ENV['BUILD_TYPE']
git_sha = `git rev-parse HEAD`.chomp
fds_src_dir = ENV['FDS_SRC_DIR']

raise "BUILD_TYPE must be set to one of 'debug' or 'release'" unless build_type
raise "FDS_SRC_DIR must be set'" unless fds_src_dir

fds_version << "~#{git_sha}"

default_version fds_version

whitelist_file "bin/*"

build do
    sync "#{fds_src_dir}/source/Build/linux-x86_64.#{build_type}/bin", "#{install_dir}/bin"
    sync "#{fds_src_dir}/source/Build/linux-x86_64.#{build_type}/lib/java", "#{install_dir}/lib/java"
    sync "#{fds_src_dir}/source/Build/linux-x86_64.#{build_type}/lib/admin-webapp", "#{install_dir}/lib/admin-webapp"
    sync "#{fds_src_dir}/source/config/etc/ssl/dev", "#{install_dir}/etc/ssl/dev"

    mkdir "#{install_dir}/lib/python2.7/dist-packages/fdslib"
    copy "#{fds_src_dir}/source/platform/python/disk_type.py", "#{install_dir}/lib/python2.7/dist-packages"
    sync "#{fds_src_dir}/source/test/fdslib", "#{install_dir}/lib/python2.7/dist-packages/fdslib", exclude: "*.pyc"

    mkdir "#{install_dir}/sbin"
    copy "#{fds_src_dir}/source/tools/redis.sh", "#{install_dir}/sbin"
    copy "#{fds_src_dir}/source/tools/coroner.py", "#{install_dir}/sbin"

    copy "#{fds_src_dir}/source/config/etc/*.conf", "#{install_dir}/etc/"
    copy "#{fds_src_dir}/source/config/etc/omenv", "#{install_dir}/etc/"
    copy "#{fds_src_dir}/source/test/formation.conf", "#{install_dir}/sbin/deploy_formation.conf"
    copy "#{fds_src_dir}/source/tools/fdsconsole.py", "#{install_dir}/sbin"
    copy "#{fds_src_dir}/source/cinder/nbdadm.py", "#{install_dir}/sbin"
    sync "#{fds_src_dir}/source/tools/fdsconsole", "#{install_dir}/sbin/fdsconsole", exclude: "*.pyc"
end
