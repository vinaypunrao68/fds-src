import shutil
import os
import TestCase
from fdslib.TestUtils import findNodeFromInv
import random


# This class removes a disk-map entry randomly
class TestRemoveDisk(TestCase.FDSTestCase):
    def __init__(self, parameters=None, diskMapPath=None, diskType=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_diskRemoval,
                                             "Testing disk removal")

        self.diskMapPath = diskMapPath
        self.diskType = diskType

    def test_diskRemoval(self):
        """
        Test Case:
        Remove a hdd/ssd chosen randomly from a given disk-map.
        """
        if (self.diskMapPath is None or self.diskType is None):
            self.log.error("Some parameters missing values {} {}".format(self.diskMapPath, self.diskType))
        self.log.info(" {} {} ".format(self.diskMapPath, self.diskType))
        if not os.path.isfile(self.diskMapPath):
            self.log.error("File {} not found".format(self.diskMapPath))
            return False
        diskMapFile = open(self.diskMapPath, "r+")
        diskEntries = diskMapFile.readlines()
        chosenDisks = filter(lambda x: self.diskType in x, diskEntries)
        chosenDisk = random.choice(chosenDisks)
        self.log.info("Disk to be removed {}".format(chosenDisk))
        diskMapFile.seek(0)
        for disk in diskEntries:
            if disk != chosenDisk:
                self.log.info(disk)
                diskMapFile.write(disk)
        diskMapFile.truncate()
        diskMapFile.seek(0)
        reRead = diskMapFile.readlines()
        self.log.info("Updated disk-map file {} \n".format(reRead))
        diskMapFile.close()
        return True


class TestStorMgrDeleteDisk(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None, disk=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_DeleteDisk,
                                             "Test to remove disk from fds mounted directory")
        self.node = node
        self.disk = disk

    def test_DeleteDisk(self):
        fdscfg = self.parameters["fdscfg"]
        self.node = findNodeFromInv(fdscfg.rt_obj.cfg_nodes, self.node)
        fds_dir = self.node.nd_conf_dict['fds_root']
        diskPath = os.path.join(fds_dir, 'dev', self.disk)
        self.log.info("Going to delete mount point {} for disk {}".format(diskPath, self.disk))
        shutil.rmtree(diskPath)
        return True
