#!/usr/bin/python
from contextlib import contextmanager
import os
import tempfile
from time import sleep
import unittest
from subprocess import Popen, PIPE
import shutil
import fds_common
import uuid

am_endpoint = ("localhost", 9988)
cs_endpoint = ("localhost", 9090)
nbd_endpoint = ("localhost", 10809)
nbdadmd_endpoint = ("localhost", 10511)


class TestExecutor:
    def __call__(self, *args, **kwargs):
        cmd_elts = list(args)
        cmd_elts[0] = self.fixNbdPath(cmd_elts[0])
        proc = Popen(cmd_elts, stdout=PIPE, stderr=PIPE)
        (stdout, stderr) = proc.communicate()
        if proc.returncode != 0:
            raise Exception("command had nonzero return code {0} (stderr: {1})".format(cmd_elts, stderr.strip()))

        return (stdout, stderr)

    def quoteIfNeeded(self, s):
        if ' ' in s or '\t' in s:
            return '"' + s + '"'
        return s

    def fixNbdPath(self, s):
        if s == 'nbdadm.py':
            return "./" + s
        else:
            return s

class TestFdsCommonOpenstack(unittest.TestCase):
    def setUp(self):
        self.services = fds_common.FDSServices(am_endpoint, cs_endpoint)
        self.nbd_server = nbd_endpoint[0] + ":" + str(nbd_endpoint[1])
        self.executor = TestExecutor()
        self.nbd_mgr = fds_common.NbdManager(self.executor)

    @contextmanager
    def volume(self, size):
        volumedata = {"name": self.randomName(), "size": size }
        self.services.create_volume("FDS", volumedata)
        yield volumedata["name"]
        self.services.delete_volume("FDS", volumedata)

    @staticmethod
    def randomName():
        return str(uuid.uuid4())

    @contextmanager
    def mount(self, dev):
        tempdir = tempfile.mkdtemp()
        self.executor("mount", dev, tempdir)
        yield tempdir
        self.executor("umount", tempdir)
        shutil.rmtree(tempdir)

    @contextmanager
    def runNbdRestServer(self):
        nbdadmd = os.path.abspath("nbdadmd.py")
        process = Popen(nbdadmd, stdout=PIPE, stderr=PIPE)
        sleep(1)
        yield "http://" + nbdadmd_endpoint[0] + ":" + str(nbdadmd_endpoint[1])
        process.kill()

    @contextmanager
    def useRemote(self, url, volume):
        yield self.nbd_mgr.attach_nbd_remote(url, self.nbd_server, volume)
        self.nbd_mgr.detach_nbd_remote(url, self.nbd_server, volume)


    def randomBits(self, length):
        with open("/dev/urandom") as random:
            return random.read(length)

    def testCreateDeleteVolume(self):
        with self.volume(1):
            pass

    def testAttachDetach(self):
        with self.volume(1) as vol:
            with self.nbd_mgr.use_nbd_local(self.nbd_server, vol):
                pass

    def testWriteThenRead(self):
        with self.volume(1) as vol:
            with self.nbd_mgr.use_nbd_local(self.nbd_server, vol) as nbd_device:
                control = self.randomBits(50000)
                with open(nbd_device, "w") as nbd_handle:
                    nbd_handle.write(control)

                with open(nbd_device, "r") as nbd_handle:
                    returnedBits = nbd_handle.read(len(control))
                    self.assertEqual(control, returnedBits)

    def testMkfs(self):
        with self.volume(1) as vol:
            with self.nbd_mgr.use_nbd_local(self.nbd_server, vol) as nbd_device:
                self.executor("mkfs.ext3", nbd_device)
                with self.mount(nbd_device) as mount_point:
                    os.makedirs(os.path.join(mount_point, "world"))
                    hello_file = os.path.join(mount_point, "world", "hello")
                    hello_output = "hello world!"

                    with open(hello_file, "w") as hello_handle:
                        hello_handle.write(hello_output)

                # remount device and verify write
                with self.mount(nbd_device) as mount_point:
                    hello_file = os.path.join(mount_point, "world", "hello")
                    with open(hello_file) as hello_handle:
                        read_line = hello_handle.readline()
                        self.assertEqual(hello_output, read_line)

    def testRemote(self):
        with self.volume(1) as vol:
            with self.runNbdRestServer() as url:
                with self.useRemote(url, vol) as nbd_device:
                    pass

if __name__ == '__main__':
    unittest.main()