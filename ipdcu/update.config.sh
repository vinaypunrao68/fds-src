#!/bin/bash

[ ${#} -ne 1 ] && echo "Usage:  ${0##*/} <pxe_server_ip>" && exit 1

pxe_server="${1}"

service_list="isc-dhcp-server tftpd-hpa"

function push_files
{ 
   for f in ${file_list}
   do
      scp ${source_path}/${f} root@${pxe_server}:${dest_path}/.
   done
}

source_path="dhcp"
dest_path="/etc/dhcp"
file_list="dhcpd.conf"
push_files

source_path="dhcp"
dest_path="/etc/default"
file_list="isc-dhcp-server"
push_files

source_path="preseed"
dest_path="/var/www/html/preseed"
file_list="node.preseed"
push_files

source_path="pxe"
dest_path="/etc"
file_list="inetd.conf"
push_files

source_path="pxe"
dest_path="/etc/default"
file_list="tftpd-hpa"
push_files

source_path="pxe"
dest_path="/var/lib/tftpboot/ubuntu-installer/amd64/boot-screens"
file_list="txt.cfg"
push_files

for s in ${service_list}
do
   ssh root@${pxe_server} service ${s} restart
done
