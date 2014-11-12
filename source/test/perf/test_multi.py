#!/usr/bin/python
import os, re, sys, paramiko
import threading

def ssh_exec(node, cmd):
    ssh = paramiko.SSHClient()
    ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    ssh.connect(node, username='root', password='passwd')
    (stdin, stdout, stderr) = ssh.exec_command(cmd)
    output = stdout.read()
    ssh.close()
    return output

def transfer_file(node, local_f, remote_f, mode):
    ssh = paramiko.SSHClient()
    ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    ssh.connect(node, username='root', password='passwd')
    ftp = ssh.open_sftp()
    if mode == "get":
        ftp.get(remote_f, local_f)
    elif mode == "put":
        ftp.put(local_f, remote_f)
    else:
        raise "Error: transfer mode can be put or get"
    ftp.close()
    ssh.close()


def task(node):
    transfer_file(node, "../traffic_gen.py", "/root/traffic_gen.py", "put")
    transfer_file(node, ".uploaded.pickle", "/root/.uploaded.pickle", "put")
    cmd = "python3 traffic_gen.py -t 30 -n 500000 -T GET -s 4096 -F 1000 -v 1 -u -N 10.1.10.222"
    output = ssh_exec(node, cmd)
    print output

nodes = ["10.1.10.139", "10.1.10.80", "10.1.10.221"]
threads = []
for n in nodes:
        t = threading.Thread(target=task, args=(n,))
        t.start()
        threads.append(t)
for t in threads:
    t.join()
