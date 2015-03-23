name "gmond-modules"

fds_src_dir = ENV['FDS_SRC_DIR']
raise "FDS_SRC_DIR must be set'" unless fds_src_dir

build do

    mkdir "#{install_dir}/embedded/lib64/ganglia/python_modules"

    copy "#{fds_src_dir}/ansible/files/deploy_fds/gmond_modules/diskstat.py", "#{install_dir}/embedded/lib64/ganglia/python_modules/diskstat.py"
    copy "#{fds_src_dir}/ansible/files/deploy_fds/gmond_modules/conf.d/diskstat.pyconf", "#{install_dir}/embedded/etc/conf.d/diskstat.pyconf"

    copy "#{fds_src_dir}/ansible/files/deploy_fds/gmond_modules/cpu_stats.py", "#{install_dir}/embedded/lib64/ganglia/python_modules/cpu_stats.py"
    copy "#{fds_src_dir}/ansible/files/deploy_fds/gmond_modules/conf.d/cpu_stats.pyconf", "#{install_dir}/embedded/etc/conf.d/cpu_stats.pyconf"

    copy "#{fds_src_dir}/ansible/files/deploy_fds/gmond_modules/mem_stats.py", "#{install_dir}/embedded/lib64/ganglia/python_modules/mem_stats.py"
    copy "#{fds_src_dir}/ansible/files/deploy_fds/gmond_modules/conf.d/mem_stats.pyconf", "#{install_dir}/embedded/etc/conf.d/mem_stats.pyconf"

    copy "#{fds_src_dir}/ansible/files/deploy_fds/gmond_modules/procstat.py", "#{install_dir}/embedded/lib64/ganglia/python_modules/procstat.py"
    copy "#{fds_src_dir}/ansible/files/deploy_fds/gmond_modules/conf.d/procstat.pyconf", "#{install_dir}/embedded/etc/conf.d/procstat.pyconf"

    copy "#{fds_src_dir}/ansible/files/deploy_fds/gmond_modules/netstats.py", "#{install_dir}/embedded/lib64/ganglia/python_modules/netstats.py"
    copy "#{fds_src_dir}/ansible/files/deploy_fds/gmond_modules/conf.d/netstats.pyconf", "#{install_dir}/embedded/etc/conf.d/netstats.pyconf"

    copy "#{fds_src_dir}/ansible/files/deploy_fds/gmond_modules/multi-interface.py", "#{install_dir}/embedded/lib64/ganglia/python_modules/multi-interface.py"
    copy "#{fds_src_dir}/ansible/files/deploy_fds/gmond_modules/conf.d/multi-interface.pyconf", "#{install_dir}/embedded/etc/conf.d/multi-interface.pyconf"

    copy "#{fds_src_dir}/ansible/files/deploy_fds/gmond_modules/tcpconn.py", "#{install_dir}/embedded/lib64/ganglia/python_modules/tcpconn.py"
    copy "#{fds_src_dir}/ansible/files/deploy_fds/gmond_modules/conf.d/tcpconn.pyconf", "#{install_dir}/embedded/etc/conf.d/tcpconn.pyconf"

    copy "#{fds_src_dir}/ansible/files/deploy_fds/gmond_modules/entropy.py", "#{install_dir}/embedded/lib64/ganglia/python_modules/entropy.py"
    copy "#{fds_src_dir}/ansible/files/deploy_fds/gmond_modules/conf.d/entropy.pyconf", "#{install_dir}/embedded/etc/conf.d/entropy.pyconf"
    
    copy "#{fds_src_dir}/ansible/files/deploy_fds/gmond_modules/vm_stats.py", "#{install_dir}/embedded/lib64/ganglia/python_modules/vm_stats.py"
    copy "#{fds_src_dir}/ansible/files/deploy_fds/gmond_modules/conf.d/vm_stats.pyconf", "#{install_dir}/embedded/etc/conf.d/vm_stats.pyconf"
end
