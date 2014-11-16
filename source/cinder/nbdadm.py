#!/usr/bin/python
from contextlib import contextmanager

import os
import subprocess
import sys
import argparse
import traceback
import fcntl
import psutil
import time

lock_file = "/tmp/nbdadm_lock"

def get_parser():
    parser = argparse.ArgumentParser()

    subparsers = parser.add_subparsers(dest='command')
    attach_parser = subparsers.add_parser("attach", help='attach a volume to an nbd device - the attached device is printed to stdout')
    attach_parser.add_argument("nbd_host", help='the host to attach to in HOST:PORT notation (10809 assumed if PORT is omitted)')
    attach_parser.add_argument("volume_name", help='the name of the volume to attach')

    detach_parser = subparsers.add_parser("detach", help='detach a volume - nothing happens if the volume is not attached')
    detach_parser.add_argument("nbd_host", nargs='?', help='detach volume for a specific host - if omitted, detach all volumes with volume_name')
    detach_parser.add_argument("volume_name", help='the name of the volume to detach')

    subparsers.add_parser("list", help='list attached volumes')
    return parser

@contextmanager
def lock():
    if lock_file is not None:
        lockfd = os.open(lock_file, os.O_CREAT)
        fcntl.flock(lockfd, fcntl.LOCK_EX)
        yield None
        fcntl.flock(lockfd, fcntl.LOCK_UN)
    else:
        yield None

def device_paths():
    for dev in os.listdir('/dev'):
        if dev.startswith('nbd') and dev[3:].isdigit():
            yield "/dev/" + dev

def consume_arg(args, arg, hasValue = False):
    if arg in args:
        val = args.index(arg)
        args.pop(val)
        if hasValue:
            return args.pop(val)
        else:
            return True
    else:
        if hasValue:
            return None
        else:
            return False

def nbd_connections():
    for p in psutil.process_iter():
        if p.name == 'nbd-client':
            try:
                args = p.cmdline()

                volume_name = consume_arg(args, "-N", True)
                if volume_name is None:
                    volume_name = consume_arg(args, "-name", True)

                consume_arg(args, '-sdp')
                consume_arg(args, '-swap')
                consume_arg(args, '-persist')
                consume_arg(args, '-nofork')
                consume_arg(args, '-block-size', True)
                consume_arg(args, '-b', True)
                consume_arg(args, '-timeout', True)
                consume_arg(args, 'nbd-client')

                # this isn't an nbd invocation we care about
                if len(args) < 2 or consume_arg(args, '-d') or volume_name is None:
                    continue

                nbd_host = args[0]
                port = 10809
                nbd_device = args[-1]
                if args.count == 3:
                    port = args[1]

                yield (p, nbd_device, nbd_host + ":" + str(port), volume_name)
            except psutil.NoSuchProcess:
                continue


def list_conn(args):
    for c in nbd_connections():
        (process, device, host, volume) = c
        print "%s\t%s\t%s" % (device, host, volume)
    return 0

def split_host(host_spec):
    port = 10809
    if ':' in host_spec:
        host_elts = host_spec.split(':', 2)
        host = host_elts[0]
        port = int(host_elts[1])
    else:
        host = host_spec

    return (host, port)

def attach(args):
    if os.geteuid() != 0:
        sys.stderr.write('you must be root to attach\n')
        return 1


    (host, port) = split_host(args.nbd_host)

    devs = device_paths()
    for conn in nbd_connections():
        (p, dev, c_host, volume) = conn
        if args.volume_name == volume and split_host(c_host) == (host, port):
            print dev
            return 0

    conns = set([d for (_, d, _, _) in nbd_connections()])
    for dev in devs:
        if dev not in conns:
            if port == 10809:
                nbd_args = ['nbd-client', '-N', args.volume_name, host, dev, '-b', '4096']
            else:
                nbd_args = ['nbd-client', '-N', args.volume_name, host, str(port), dev, '-b', '4096']

            nbd_client = subprocess.Popen(nbd_args, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
            # add timeout?
            (stdout, stderr) = nbd_client.communicate()

            if nbd_client.returncode == 0:
                print dev
                return 0
            elif 'Server closed connection' in stderr:
                sys.stderr.write('server closed connection - does specified volume exist?\n')
                return 2
            elif 'Socket failed: Connection refused' in stderr:
                sys.stderr.write('connection refused by server - is nbd host up?\n')
                return 3
            else:
                continue

    sys.stderr.write('no eligible nbd devices found\n')
    return 4

@staticmethod
def safekill(process):
    try:
        process.kill()
    except psutil.NoSuchProcess as e:
        pass


def disconnect(dev, process):
    dproc = None
    try:
        dproc = psutil.Popen('nbd-client -d %s' % dev, shell=True, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
        dproc.wait(3)
    except psutil.TimeoutExpired as e:
        if dproc is not None:
            safekill(dproc)

    for i in xrange(0, 100):
        if not process.is_running():
            break

        try:
            process.kill()
        except psutil.NoSuchProcess:
            pass

        time.sleep(0.05)


def detach(args):
    if os.geteuid() != 0:
        sys.stderr.write('you must be root to detach\n')
        return 2

    retval = 0
    attempted_detach = False
    for c in nbd_connections():
        (process, dev, host, volume) = c
        if args.volume_name == volume and (args.nbd_host is None or split_host(args.nbd_host) == split_host(host)):
            attempted_detach = True
            disconnect(dev, process)
            if process.is_running():
                sys.stderr.write('could not stop nbd-client[%d] on %s\n' % (process.pid, dev))
                retval = 1

    if not attempted_detach:
        print 'nothing to detach'
    return retval



def main(argv = sys.argv):
    args = list(argv)
    args.pop(0)
    parser = get_parser()
    result = parser.parse_args(args)

    try:
        if result.command == 'list':
            return list_conn(result)
        elif result.command == 'attach':
            with lock():
                return attach(result)
        elif result.command == 'detach':
            with lock():
                return detach(result)
    except Exception as e:
        sys.stderr.write(e.message + '\n')
        tb = traceback.format_exc()
        sys.stderr.write(tb + '\n')
        return 255


if __name__ == '__main__':
    sys.exit(main())



