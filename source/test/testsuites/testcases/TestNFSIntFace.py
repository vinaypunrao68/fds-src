#!/usr/bin/python
#
# Copyright 2016 by Formation Data Systems, Inc.

import unittest
import sys
import TestCase
from fdslib.TestUtils import findNodeFromInv
from fdslib.TestUtils import get_node_service
from fdslib.TestUtils import get_resource
from os.path import expanduser
from fabric.context_managers import lcd
from fabric.operations import local

try:
    import libnfs
except ImportError:
    p = expanduser("~") + '/' + 'libnfs_python' + '/' + 'libnfs/'
    with lcd(p):
        local("make clean && make")



# This class contains the attributes and methods to test libnfs interface to upload a given file.
# You must have successfully created the NFS where the file is to be loaded and
# created NFS context using class `TestNFSAttach`.
#
# @param inputfiles: comma separated list of files to be loaded on NFS share.
# @param verify: If verify is true then verify written data by reading from mount point. default value is true,
class TestNFSLoadVerifiableFiles(TestCase.FDSTestCase):
    def __init__(self, parameters=None, inputfiles=None, verify="true"):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_NfsLoadVerifiableFiles,
                                             "Upload a given file in NFS ")

        self.passedInputFiles = inputfiles
        if verify == "true":
            self.passedVerify = True
        else:
            self.passedVerify = False

    def test_NfsLoadVerifiableFiles(self):
        if not Helper.checkNFSInfo(self):
            return False
        nfs = self.parameters["nfs"]
        input_files = self.passedInputFiles.split(",")

        for each_file in input_files:
            # If our HLQ is "RESOURCES", then we'll pull the file from the SysTest framework's resources repository.
            if os.path.dirname(each_file) == "RESOURCES":
                source_path = get_resource(self, os.path.basename(each_file))
            else:
                source_path = each_file

            # Write file from source_path to NFS mount point
            source_file = open(source_path).read()
            soure_file_Name = os.path.basename(source_path)
            a = nfs.open('/' + soure_file_Name, mode='w+')
            a.write(source_file)
            a.close()

            # If verify is true then only verify written data to mount point.
            if self.passedVerify is True:
                read_file = nfs.open('/' + soure_file_Name, mode='r').read()
                if read_file == source_file:
                    self.log.info('OK: Put {} on NFS share'.format(each_file))
                else:
                    self.log.error('Failed : Put {0} in NFS volume {1} '.format(each_file))
                    return False
        return True


# This class contains the attributes and methods to test
# the FDS libnfs interface to verify a given file is same as uploaded file.
# Here you put (say) falcor.png then read from mount point "/falcor.png" and original file are compared.
# You must have successfully created the NFS where the file is to be loaded and
# created NFS context using class `TestNFSAttach`.
#
# @param inputfiles: comma separated list of files on nfs share which will be verified against the source files.
class TestNFSCheckVerifiableFiles(TestCase.FDSTestCase):
    def __init__(self, parameters=None, inputfiles=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_NFSCheckVerifiableFiles,
                                             "Check verifiable file in NFS bucket")

        self.passedInputFiles = inputfiles

    def test_NFSCheckVerifiableFiles(self):
        if not Helper.checkNFSInfo(self):
            return False
        nfs = self.parameters["nfs"]
        input_files = self.passedInputFiles.split(",")
        for each_file in input_files:

            # If our HLQ is "RESOURCES", then we'll pull the file from the SysTest framework's resources repository.
            if os.path.dirname(self.passedInputFiles) == "RESOURCES":
                source_path = get_resource(self, os.path.basename(self.passedInputFiles))
            else:
                source_path = self.passedInputFiles

            source_file = open(source_path).read()
            soure_file_Name = os.path.basename(source_path)
            read_file = nfs.open('/' + soure_file_Name, mode='r').read()
            if read_file == source_file:
                self.log.info('OK: NFS Verifiable file {0} matched'.format(each_file))
            else:
                self.log.error('Fail : NFS Verifiable {} did not match'.format(each_file))
                return False
        return True


# This class contains the attributes and methods to test
# the FDS libnfs interface to delete a given file from nfs file system.
# You must have successfully created the NFS from where the input file is to be deleted and
# created NFS context using class `TestNFSAttach`.
#
# @param inputfiles: comma separated list of files on nfs share which will be deleted.
# @param verify: If verify is true then confirm status is 0. Deafault value is true.
class TestNFSDeleteVerifiableFiles(TestCase.FDSTestCase):
    def __init__(self, parameters=None, verify="true", inputfiles=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_NfsDeleteVerifiableFiles,
                                             "Delete passed file from NFS")

        self.passedInputFiles = inputfiles
        if verify == "true":
            self.passedVerify = True
        else:
            self.passedVerify = False

    def test_NfsDeleteVerifiableFiles(self):
        if not Helper.checkNFSInfo(self):
            return False
        nfs = self.parameters["nfs"]
        input_files = self.passedInputFiles.split(",")
        for each_file in input_files:
            source_name = each_file
            nfs.unlink('/' + source_name)

            # If verify is true then only confirm that file is deleted
            if self.passedVerify is True:
                try:
                    nfs.open('/' + source_name, mode='r').read()
                except ValueError:
                    self.log.info("OK: {0} file is deleted from NFS share".format(source_name))
                else:
                    self.log.error("Failed: Deleting file {0} from NFS share".format(each_file))
                    return False
        return True


# In the context-full mode you will first need to mount an NFS share and create
# an NFS context before you can access any files. This NFS context is then used
# for all future access to files in that mount point. This class adds NFS context in parameters.
class TestNFSAttach(TestCase.FDSTestCase):
    def __init__(self, parameters=None, bucket=None, hotam=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_NFSAttach,
                                             "Delete passed file from given NFS bucket")

        self.passedBucket = bucket
        self.passedHotAm = hotam

    def test_NFSAttach(self):
        fdscfg = self.parameters["fdscfg"]
        om_node = fdscfg.rt_om_node
        node_service = get_node_service(self, om_node.nd_conf_dict['ip'])
        hot_am_node = findNodeFromInv(fdscfg.rt_obj.cfg_nodes, self.passedHotAm)
        status = hot_am_node.nd_populate_metadata(om_node=om_node)
        hot_am_node_id = int(hot_am_node.nd_uuid, 16)
        hot_am_node_cli = node_service.get_node(hot_am_node_id)
        hot_am_ip = str(hot_am_node_cli.address.ipv4address)

        nfs_url = "nfs://" + hot_am_ip + "/" + self.passedBucket
        nfs = libnfs.NFS(nfs_url)
        try:
            assert nfs.error is None
        except:
            self.log.error('Failed to mount on NFS volume {}'.format(self.passedBucket))
            return False

        if not "nfs" in self.parameters:
            self.parameters["nfs"] = libnfs.NFS(nfs_url)
        else:
            self.parameters["nfs"]._nfs = nfs
        return True


class Helper:
    @staticmethod
    def checkNFSInfo(self):
        if "nfs" in self.parameters is None:
            self.log.error("No NFS context")
            return False
        self.log.info("NFS server ip is {}, and path is {}".format(self.parameters["nfs"]._url.server,self.parameters["nfs"]._url.path))
        return True


if __name__ == '__main__':
    TestCase.FDSTestCase.fdsGetCmdLineConfigs(sys.argv)
    unittest.main()

