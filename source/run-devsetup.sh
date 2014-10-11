chef_client_binary=/opt/chef/bin/chef-client
chef_client_config=/tmp/chef-client-config.rb

[ -e ${chef_client_binary} ] && echo "chef-client already installed, skipping" || curl -L https://www.getchef.com/chef/install.sh | sudo bash

[ -e ${chef_client_config} ] && echo "chef-client config already generated, skipping" || echo "chef_repo_path \"`pwd`/../chef\"\ncookbook_path [ \"`pwd`/../chef/cookbooks\", \"`pwd`/../chef/site-cookbooks\" ]" > ${chef_client_config}

sudo chef-client --config ${chef_client_config} -z -o fds_setup::dev_setup
