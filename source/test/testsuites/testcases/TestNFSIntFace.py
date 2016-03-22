import TestCase
import fdslib.TestUtils as TestUtils
from fdslib.TestUtils import findNodeFromInv
from fdslib.TestUtils import get_node_service
from fabric.contrib.files import *
import libnfs
import ntpath


# This class contains the attributes and methods to test
# the FDS libnfs interface to upload a given file.
# Here say you are uploading file falcor.png then mount point is /falcor.png
# You must have successfully created nfs bucket
# and 'passedInputFile' should be available in given RESOURCES dir unless path is unspecified
class TestNFSLoadVerifiableFile(TestCase.FDSTestCase):
    def __init__(self, parameters=None, bucket=None, hotam=None, inputfile=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_NfsLoadVerifiableFiles,
                                             "Upload file in NFS bucket")

        self.passedBucket = bucket
        self.passedInputFile = inputfile
        self.passedHotAm =hotam

    def test_NfsLoadVerifiableFiles(self):
        nfs = nfs_connect(self)
        input_files = self.passedInputFile.split(",")
        for each_file in input_files:
            qualifiedFile_path = TestUtils.get_resource(self, os.path.basename(each_file))
            putFile = open(qualifiedFile_path)
            input_file = putFile.read()
            qualifiedFile_Name = get_file_name_from_path(qualifiedFile_path)
            a = nfs.open('/'+qualifiedFile_Name, mode='w+')
            a.write(input_file)
            a.close()
            read_file =nfs.open('/'+qualifiedFile_Name, mode='r').read()
            if read_file == input_file:
                self.log.info('OK: File {0} in NFS volume {1}'.format(each_file, self.passedBucket))
            else:
                self.log.error('Failed : Put {0} in NFS volume {1} '.format(each_file, self.passedBucket))
                return False
        return True


# This class contains the attributes and methods to test
# the FDS libnfs interface to verify a given file is same as uploaded file.
# Here you put (say) falcor.png then read from mount point "/falcor.png" and original file are compared.
# You must have successfully created nfs bucket
# and 'passedInputFile' should be available in given RESOURCES dir unless path is unspecified
class TestNFSCheckVerifiableFile(TestCase.FDSTestCase):
    def __init__(self, parameters=None, bucket=None, hotam=None, inputfile='RESOURCES/falcor.png'):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_NfsVerify,
                                             "Check verifiable file in NFS bucket")

        self.passedBucket = bucket
        self.passedInputFile = inputfile
        self.passedHotAm =hotam

    def test_NfsVerify(self):
        nfs = nfs_connect(self)
        input_files = self.passedInputFile.split(",")
        for each_file in input_files:
            qualifiedFile_path = TestUtils.get_resource(self, os.path.basename(each_file))
            putFile = open(qualifiedFile_path)
            input_file = putFile.read()
            qualifiedFile_Name = get_file_name_from_path(qualifiedFile_path)
            read_file = nfs.open('/'+qualifiedFile_Name, mode='r').read()
            if read_file == input_file:
                self.log.info('OK: NFS Verifiable file {0} matched'.format(each_file))
            else:
                self.log.error('Fail : File {0} mismatch'.format(each_file))
                return False
        return True


# This class contains the attributes and methods to test
# the FDS libnfs interface to delete a given file from passed nfs bucket.
# You must have successfully created nfs bucket
# and 'passedInputFile' should be available in given RESOURCES dir unless path is unspecified
class TestNFSDeleteVerifiableFiles(TestCase.FDSTestCase):
    def __init__(self, parameters=None, bucket=None, hotam=None, inputfile=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_NfsDeleteVerifiableFiles,
                                             "Delete passed file from given NFS bucket")

        self.passedBucket = bucket
        self.passedInputFile = inputfile
        self.passedHotAm =hotam

    def test_NfsDeleteVerifiableFiles(self):
        nfs = nfs_connect(self)
        input_files = self.passedInputFile.split(",")
        for each_file in input_files:
            qualifiedFile_path = TestUtils.get_resource(self, os.path.basename(each_file))
            qualifiedFile_Name = get_file_name_from_path(qualifiedFile_path)
            status = nfs.unlink('/'+qualifiedFile_Name)
            if status != 0:
                self.log.error("Failed: Deleting file {0} from NFS volume {} failed ".format(each_file, self.passedBucket))
                print ('')
                return False
        return True


def nfs_connect(self):
    fdscfg = self.parameters["fdscfg"]
    om_node = fdscfg.rt_om_node
    node_service = get_node_service(self, om_node.nd_conf_dict['ip'])
    hot_am_node = findNodeFromInv(fdscfg.rt_obj.cfg_nodes, self.passedHotAm)
    status = hot_am_node.nd_populate_metadata(om_node=om_node)
    hot_am_node_id = int(hot_am_node.nd_uuid, 16)
    hot_am_node_cli = node_service.get_node(hot_am_node_id)
    hot_am_ip= str(hot_am_node_cli.address.ipv4address)

    nfs_url = "nfs://"+hot_am_ip+"/"+self.passedBucket
    nfs = libnfs.NFS(nfs_url)
    try:
        assert nfs.error is None
    except:
        print 'Failed to mount on NFS volume {}'.format(self.passedBucket)
    return nfs


def get_file_name_from_path(qualifiedFile_path):
        head, tail = ntpath.split(qualifiedFile_path)
        return tail or ntpath.basename(head)
