# fds-cli
FDS CLI tool for interacting with a cluster

## Getting Started
Install fdscli package:  
`sudo pip install fsdcli`

Run it:  
`fds`

You should be prompted for a few values (hostname, user, pass, port).

Or, you can create a config file for your preferred cluster and avoid the prompts:
```
cat <<EOF >> ./fdscli.conf
[connection]
hostname=1.2.3.4
username=admin
password=admin
port=7777
EOF
```
