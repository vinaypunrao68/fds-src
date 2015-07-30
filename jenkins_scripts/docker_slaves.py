#!/usr/bin/env python
# Manage Jenkins docker containers
# requires pip install docker-py

import docker
import optparse
import inspect
import subprocess
from optparse import OptionParser

parser = OptionParser()
parser.set_defaults()

parser.add_option("-l", "--list", action="store_true", dest="list")
parser.add_option("-d", "--destroy", metavar="NAME" )
parser.add_option("-s", "--start", metavar="NAME" )
parser.add_option("-r", "--restart", metavar="NAME" )
parser.add_option("-k", "--kill", metavar="NAME" )
parser.add_option("-c", "--connect", metavar="NAME" )

(options, args) = parser.parse_args()

DOCKER_PORT=2375
DOCKER_SLAVES = {
        "fre-build-01": {
            "builder01": "10051"
            },
        "fre-build-02": {
            "builder03": "10053"
            },
        "fre-build-03": {
            "builder05": "10055"
            },
        "fre-build-04": {
            "builder07": "10057",
            "builder08": "10058"
            },
        "juno-compute1": {
            "builder09": "10059",
            },
        "juno-control": {
            "builder10": "10060"
            },
        "10.3.121.72": {
            "builder1000": "11000"
            },
        "10.3.121.71": {
            "builder1001": "11001"
            },
        "10.3.121.74": {
            "builder1002": "11002"
            },
        "10.3.121.73": {
            "builder1003": "11003"
            }
        }
IMAGE="registry.formationds.com:5000/fds_dev"

def docker_connect(url):
    return(docker.Client(base_url=url, version='auto'))

def get_container_by_name(conn, name, die=True):
    containers = conn.containers(all=True)
    for container in containers:
        if '/%s' % name in container['Names']:
            return container
    if die:
        print "No container found named %s" % name
        exit(1)
    else:
        return False

def get_slave_host_url(name):
    for host, slaves in DOCKER_SLAVES.iteritems():
        for slave, port in slaves.iteritems():
            if slave == name:
                return "tcp://%s:%d" % (host, DOCKER_PORT)
    print "Slave %s not found in slave configuration list" % name
    exit(1)

def get_slave_port(name):
    for host, slaves in DOCKER_SLAVES.iteritems():
        for slave, port in slaves.iteritems():
            if slave == name:
                return port

def get_slave_host(name):
    for host, slaves in DOCKER_SLAVES.iteritems():
        for slave, port in slaves.iteritems():
            if slave == name:
                return host

# Get a docker connection for given slave
def get_conn(name):
    url = get_slave_host_url(name)
    conn = docker_connect(url)
    return conn

def create_container(conn, name):
    container = conn.create_container(IMAGE, name=name, volumes=["/mnt/ccache", "/tmp/nbdadm"])
    return(container)

def start_slave(name):
    print "Starting slave %s" % name
    ssh_port = get_slave_port(name)
    shm_dir = "/dev/shm/ccache-%s" % name
    nbd_lock_dir = "/tmp/nbdadm/"
    conn = get_conn(name)
    if not exists(conn, name):
        container = create_container(conn, name)
    else:
        container = get_container_by_name(conn, name)

    conn.start(container, port_bindings={22: ssh_port}, publish_all_ports=True,
            privileged=True, binds={
                shm_dir: {
                    'bind': '/mnt/ccache',
                    'ro': False
                },
                nbd_lock_dir: {
                    'bind': '/tmp/nbdadm',
                    'ro': False
                }
            })

def restart_slave(name):
    print "Restarting slave %s" % name
    conn = get_conn(name)
    container = get_container_by_name(conn, name)
    conn.restart(container)
    status = conn.inspect_container(container)['Status']
    print "Restarted slave %s Status: %s" % (name, status)

def destroy_slave(name):
    print "Destroying slave %s" % (name)
    conn = get_conn(name)
    container = get_container_by_name(conn, name)
    conn.kill(container)
    conn.remove_container(container)
    print "Slave %s destroyed" % name

def kill_slave(name):
    print "Killing slave %s" % name
    conn = get_conn(name)
    container = get_container_by_name(conn, name)
    if container:
        conn.kill(container)
        # Check status
        if is_running(conn, container):
            print "Failed to kill container"
        else:
            print "Container killed"
    else:
        print "%s not found" % name

def connect_slave(name):
    print "Connecting to slave %s" % name
    ssh_port = get_slave_port(name)
    ssh_host = get_slave_host(name)
    cmd = "ssh -p %s pairing@%s" % (ssh_port, ssh_host)
    subprocess.call(cmd, shell=True)

def get_container_status(conn, container):
    return conn.inspect_container(container)["Status"]

def is_running(conn, container):
    status = conn.inspect_container(container)['State']['Running'] | False
    return status

def exists(conn, name):
    container = get_container_by_name(conn, name, die=False)
    if container:
        return True
    else:
        return False

def list_slaves():
    for host, slaves in DOCKER_SLAVES.iteritems():
        for slave, port in sorted(slaves.iteritems()):
            conn = get_conn(slave)
            slave_container = get_container_by_name(conn, slave, die=False)
            if slave_container:
                print "Name: %s Port: %s Status: %s" % (slave, port, slave_container['Status'])
            else:
                print "Name: %s not found" % slave

def main():

    if options.list:
        list_slaves()
    elif options.start:
        start_slave(options.start)
    elif options.kill:
        kill_slave(options.kill)
    elif options.restart:
        restart_slave(options.restart)
    elif options.destroy:
        destroy_slave(options.destroy)
    elif options.connect:
        connect_slave(options.connect)

if __name__ == "__main__":
    main()
