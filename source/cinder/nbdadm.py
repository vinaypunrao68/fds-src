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

class nbdError(Exception):
    def __init__(self, value):
        self.value = value
    def __str__(self):
        return repr(self.value)

class nbdlib:

    def __init__(self):
        self.lock_file = "/tmp/nbdadm_lock"
        self.host = None
        self.port = 10809


    @contextmanager
    def lock(self):
        if lock_file is not None:
            lockfd = os.open(lock_file, os.O_CREAT)
            fcntl.flock(lockfd, fcntl.LOCK_EX)
            yield None
            fcntl.flock(lockfd, fcntl.LOCK_UN)
        else:
            yield None

    def __device_paths(self):
        for dev in os.listdir('/dev'):
            if dev.startswith('nbd') and dev[3:].isdigit():
                yield "/dev/" + dev

    def __insmod_nbd():
        return os.system("modprobe nbd -q") == 0

    def __consume_arg(args, arg, hasValue = False):
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

    def __nbd_connections(self):
        for p in psutil.process_iter():
            if p.name() == 'nbd-client':
                try:
                    args = p.cmdline()

                    volume_name = self.__consume_arg(args, "-N", True)
                    if volume_name is None:
                        volume_name = self.__consume_arg(args, "-name", True)

                    self.__consume_arg(args, '-sdp')
                    self.__consume_arg(args, '-swap')
                    self.__consume_arg(args, '-persist')
                    self.__consume_arg(args, '-nofork')
                    self.__consume_arg(args, '-block-size', True)
                    self.__consume_arg(args, '-b', True)
                    self.__consume_arg(args, '-timeout', True)
                    self.__consume_arg(args, '-t', True)
                    self.__consume_arg(args, 'nbd-client')

                    # this isn't an nbd invocation we care about
                    if len(args) < 2 or self.__consume_arg(args, '-d') or volume_name is None:
                        continue

                    nbd_host = args[0]
                    port = 10809
                    nbd_device = args[-1]
                    if args.count == 3:
                        port = args[1]

                    yield (p, nbd_device, nbd_host, port, volume_name)
                except psutil.NoSuchProcess:
                    continue

    def __safekill(self, process):
        try:
            process.kill()
        except psutil.NoSuchProcess as e:
            pass

    def __disconnect(dev, process):
        dproc = None
        try:
            dproc = psutil.Popen('nbd-client -d %s' % dev, shell=True, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
            dproc.wait(3)
        except psutil.TimeoutExpired as e:
            if dproc is not None:
                self.__safekill(dproc)

        for i in xrange(0, 100):
            if not process.is_running():
                break

            try:
                process.kill()
            except psutil.NoSuchProcess:
                pass

            time.sleep(0.05)

    def detach(self, volume, host = None):
        if os.geteuid() != 0:
            raise nbdError({"message":"you must be root to detach", "code":1})

        attempted_detach = False
        for c in self.__nbd_connections():
            (process, dev, c_host, port, c_volume) = c
            if volume == c_volume and (host is None or host == c_host):
                attempted_detach = True
                self.__disconnect(dev, process)
                if process.is_running():
                    error_str = "could not stop nbd-client[%d] on %s" % (process.pid, dev)
                    raise nbdError({"message":error_str, "code":2})

        return attempted_detach

    def attach(self, host, port, vol_name, use_c=False):
        # Check that user is root
        if os.geteuid() != 0:
            raise nbdError({"message":"you must be root to attach", "code":1})

        self.host = host
        if port != None:
            self.port = port

        devs = list(__device_paths())
        if len(devs) == 0:
            if not self.__insmod_nbd():
                raise nbdError({"message":"no nbd devices found and modprobe nbd failed", "code":5})

            devs = list(self.__device_paths())

        for conn in self.__nbd_connections():
            (p, dev, c_host, c_port, volume) = conn
            if vol_name == volume and (c_host, c_port) == (host, port):
                return dev

        conns = set([d for (_, d, _, _, _) in self.__nbd_connections()])
        ret_dev = None
        for dev in devs:
            if dev not in conns:
                if use_c:
                    try:
                        with open(os.devnull, 'wb') as devnull:
                            cproc = psutil.Popen('nbd-client -c %s' % dev, shell=True, stderr=devnull, stdout=devnull)
                        exit = cproc.wait(3)
                        if exit != 1:
                            continue

                    except psutil.TimeoutExpired as e:
                        continue


                if port == 10809:
                    nbd_args = ['nbd-client', '-N', args.volume_name, host, dev, '-b', '4096', '-t', '5']
                else:
                    nbd_args = ['nbd-client', '-N', args.volume_name, host, str(port), dev, '-b', '4096', '-t', '5']

                nbd_client = subprocess.Popen(nbd_args, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
                # add timeout?
                (stdout, stderr) = nbd_client.communicate()

                if nbd_client.returncode == 0:
                    ret_dev = dev
                    break
                elif 'Server closed connection' in stderr:
                    raise nbdError({"message":"server closed connection - does specified volume exist?", "code":2})

                elif 'Socket failed: Connection refused' in stderr:
                    raise nbdError({"message":"connection refused by server - is nbd host up?", "code":3})

                else:
                    continue

        return ret_dev

    def list_conn(self):
        conn_list = []
        for c in self.__nbd_connections():
            (process, device, host, port, volume) = c
            conn_list.append((device, host, port, volume))
        return conn_list



def get_parser():
    parser = argparse.ArgumentParser()

    parser.add_argument("--lockfile", dest='lockfile', default='/tmp/nbdadm_lock', help='set the lockfile')

    subparsers = parser.add_subparsers(dest='command')
    attach_parser = subparsers.add_parser("attach", help='attach a volume to an nbd device - the attached device is printed to stdout')
    attach_parser.add_argument("nbd_host", help='the host to attach to in HOST:PORT notation (10809 assumed if PORT is omitted)')
    attach_parser.add_argument("volume_name", help='the name of the volume to attach')
    attach_parser.add_argument("--use-c", dest="should_use_c", action="store_const", const=True, default=False,
                               help='use nbd-client -c in addition to examining the running processes to determine if a device can be used')

    detach_parser = subparsers.add_parser("detach", help='detach a volume - nothing happens if the volume is not attached')
    detach_parser.add_argument("nbd_host", nargs='?', help='detach volume for a specific host - if omitted, detach all volumes with volume_name')
    detach_parser.add_argument("volume_name", help='the name of the volume to detach')

    subparsers.add_parser("list", help='list attached volumes')
    return parser

def split_host(host_spec):
    port = 10809
    if ':' in host_spec:
        host_elts = host_spec.split(':', 2)
        host = host_elts[0]
        port = int(host_elts[1])
    else:
        host = host_spec

    return (host, port)

def main(argv = sys.argv):
    args = list(argv)
    args.pop(0)
    parser = get_parser()
    result = parser.parse_args(args)

    nbdlib_inst = nbdlib()
    nbdlib_inst.lock_file = result.lockfile
    return_val = 0

    try:
        if result.command == 'list':
            for conn in nbdlib_inst.list_conn():
                (device, host, port, volume) = conn
                print "%s\t%s:%s\t%s" % (device, host, str(port), volume)

        elif result.command == 'attach':
            with nbdlib_inst.lock():
                (host, port) = split_host(result.nbd_host)
                dev = nbdlib_inst.attach(host, port, result.volume_name, result.should_use_c)
                if dev is None:
                    sys.stderr.write('no eligible nbd devices found\n')
                    return_val = 4
                else:
                    print dev

        elif result.command == 'detach':
            with nbdlib_inst.lock():
                (host, port) = split_host(result.nbd_host)
                if not detach(result.volume_name, host):
                    print 'nothing to detach'

    except nbdError as e:
        details = e.args[0]
        sys.stderr.write(details["message"])
        return_val = details["code"]

    except Exception as e:
        sys.stderr.write(e.message + '\n')
        tb = traceback.format_exc()
        sys.stderr.write(tb + '\n')
        return_val = 255

    return return_val


if __name__ == '__main__':
    sys.exit(main())

