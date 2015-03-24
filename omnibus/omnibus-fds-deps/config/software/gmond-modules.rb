# These options are required for all software definitions
name "gmond-modules"

fds_src_dir = ENV['FDS_SRC_DIR']
raise "FDS_SRC_DIR must be set'" unless fds_src_dir

default_version "0.1.0"

whitelist_file "bin/*"

build do
    mkdir "#{install_dir}/embedded/lib64/ganglia/python_modules"
    mkdir "#{install_dir}/embedded/etc/conf.d"
    sync "#{fds_src_dir}/ansible/files/deploy_fds/gmond_modules", "#{install_dir}/embedded/lib64/ganglia/python_modules", exclude: "*.pyc"
    sync "#{fds_src_dir}/ansible/files/deploy_fds/gmond_modules/conf.d", "#{install_dir}/embedded/etc/conf.d"
end
