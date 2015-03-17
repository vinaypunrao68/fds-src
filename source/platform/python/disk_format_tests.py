#!/usr/bin/env python

# Unit tests for disk_format.py

import unittest
import disk_format

class extendedFstabTest (unittest.TestCase):

    def testFstabInit (self):
        fstab = disk_format.extendedFstab()
        self.assertFalse (fstab.altered)

    def testFstabAdd (self):
        fstab = disk_format.extendedFstab()
        fstab.read ('test_data/ExtendedFstabTest.unit')
        fstab.add_mount_point ('# This is a comment')
        self.assertTrue (fstab.altered)

    def testFstabBackupNotAltered (self):
        fstab = disk_format.extendedFstab()
        fstab.read ('test_data/ExtendedFstabTest.unit')
        self.assertFalse (fstab.backup_if_altered('test_data/ExtendedFstabTest.unit.updated'))

    def testFstabBackupAltered (self):
        fstab = disk_format.extendedFstab()
        fstab.read ('test_data/ExtendedFstabTest.unit.altered')
        fstab.add_mount_point ('# This is a comment')
        self.assertTrue (fstab.backup_if_altered('test_data/ExtendedFstabTest.unit.altered'))

    def testFstabRemoveMountPointThatExists (self):
        fstab = disk_format.extendedFstab()
        fstab.read ('test_data/ExtendedFstabTest.unit')
        fstab.remove_mount_point_by_uuid ('e1b56c44-dddd-4e4f-9bd5-73ddddddd707')
        self.assertTrue (fstab.altered)

    def testFstabRemoveMountPointThatDoesNotExist (self):
        fstab = disk_format.extendedFstab()
        fstab.read ('test_data/ExtendedFstabTest.unit')
        fstab.remove_mount_point_by_uuid ('bbbbbbbb-dddd-4e4f-9bd5-73ddddddd707')
        self.assertFalse (fstab.altered)


class diskTest (unittest.TestCase):

    TEST_PATH = "/dev/foo"
    TEST_CAPACITY = "3006"

    def testDiskInitSSD (self):                       # SAS, interface doesn't matter
        d = disk_format.Disk ('/dev/sda1', True, True, disk_format.Disk.DISK_TYPE_SSD, disk_format.Disk.SAS_INTERFACE, 200)
        self.assertIsNone (d.marker)

    def testDiskInitHDDSAS (self):                    # spining disk on SAS
        d = disk_format.Disk ('/dev/sda1', True, True, disk_format.Disk.DISK_TYPE_HDD, disk_format.Disk.SAS_INTERFACE, 200)
        self.assertIsNone (d.marker)

    def testDiskInitHDDSATA (self):                   # spining disk on SATA
        d = disk_format.Disk ('/dev/sda1', True, True, disk_format.Disk.DISK_TYPE_HDD, 'NA', 200)
        self.assertIsNone (d.marker)

    def testDiskInitDiskInvalid (self):                   # Bad disk type
        with self.assertRaises (SystemExit) as cm:
            d = disk_format.Disk ('/dev/sda1', True, True, 'Monkey', 'NA', 200)
        self.assertEqual (cm.exception.code, 8)

    def testDiskgetPath (self):        
        d = disk_format.Disk (diskTest.TEST_PATH, True, True, disk_format.Disk.DISK_TYPE_HDD, 'NA', 200)
        self.assertEqual (diskTest.TEST_PATH, d.get_path())

    def testDiskgetType (self):         
        d = disk_format.Disk (diskTest.TEST_PATH, True, True, disk_format.Disk.DISK_TYPE_HDD, 'NA', 200) 
        self.assertEqual (disk_format.Disk.DISK_TYPE_HDD, d.get_type())

    def testDiskgetCapacity (self):      
        d = disk_format.Disk (diskTest.TEST_PATH, True, True, disk_format.Disk.DISK_TYPE_HDD, 'NA', diskTest.TEST_CAPACITY)
        self.assertEqual (diskTest.TEST_CAPACITY, d.get_capacity())

    def testDiskgetIndex (self):          
        d = disk_format.Disk (diskTest.TEST_PATH, True, True, disk_format.Disk.DISK_TYPE_HDD, 'NA', diskTest.TEST_CAPACITY)
        self.assertTrue (d.get_index())

    def testDiskgetIndex2 (self):          
        d = disk_format.Disk (diskTest.TEST_PATH, True, False, disk_format.Disk.DISK_TYPE_HDD, 'NA', diskTest.TEST_CAPACITY)
        self.assertFalse (d.get_index())

    def testDiskgetOsUsage (self):          
        d = disk_format.Disk (diskTest.TEST_PATH, True, True, disk_format.Disk.DISK_TYPE_HDD, 'NA', diskTest.TEST_CAPACITY)
        self.assertTrue (d.get_os_usage())

    def testDiskgetOsUsage2 (self):          
        d = disk_format.Disk (diskTest.TEST_PATH, False, False, disk_format.Disk.DISK_TYPE_HDD, 'NA', diskTest.TEST_CAPACITY)
        self.assertFalse (d.get_os_usage())

    def testDiskSmFlag (self):                
        d = disk_format.Disk (diskTest.TEST_PATH, False, False, disk_format.Disk.DISK_TYPE_HDD, 'NA', diskTest.TEST_CAPACITY)
        self.assertFalse (d.sm_flag)
        d.set_sm_flag()
        self.assertTrue (d.sm_flag)

    def testDiskDmFlag (self):                
        d = disk_format.Disk (diskTest.TEST_PATH, False, False, disk_format.Disk.DISK_TYPE_HDD, 'NA', diskTest.TEST_CAPACITY)
        self.assertFalse (d.dm_flag)
        d.set_dm_flag()
        self.assertTrue (d.dm_flag)

    def testDiskCheckForMarkerOS (self):        
        d = disk_format.Disk (diskTest.TEST_PATH, True, True, disk_format.Disk.DISK_TYPE_HDD, 'NA', 200)
        self.assertFalse (d.check_for_fds())

    def testDiskCheckForMarkerPresent (self):        
        d = disk_format.Disk (diskTest.TEST_PATH, False, True, disk_format.Disk.DISK_TYPE_HDD, 'NA', 200)
        d.marker = disk_format.DISK_MARKER
        self.assertTrue (d.check_for_fds())

    def testDiskCheckForMarkerNotPresent (self):        
        d = disk_format.Disk (diskTest.TEST_PATH, False, True, disk_format.Disk.DISK_TYPE_HDD, 'NA', 200)
        self.assertFalse (d.check_for_fds())


'''


    def testDiskgetType (self):                   # spining disk on SATA
        d = disk_format.Disk ('/dev/sda1', True, True, disk_format.Disk.DISK_TYPE_HDD, 'NA', 200)
        self.assertIsNone (d.marker)

    def testDiskgetCapacity (self):                   # spining disk on SATA
        d = disk_format.Disk ('/dev/sda1', True, True, disk_format.Disk.DISK_TYPE_HDD, 'NA', 200)
        self.assertIsNone (d.marker)

    def testDiskgetOsUsage (self):                   # spining disk on SATA
        d = disk_format.Disk ('/dev/sda1', True, True, disk_format.Disk.DISK_TYPE_HDD, 'NA', 200)
        self.assertIsNone (d.marker)

    def testDiskgetIndex (self):                   # spining disk on SATA
        d = disk_format.Disk ('/dev/sda1', True, True, disk_format.Disk.DISK_TYPE_HDD, 'NA', 200)
        self.assertIsNone (d.marker)

    def testDiskCheckForFds (self):                   # spining disk on SATA
        d = disk_format.Disk ('/dev/sda1', True, True, disk_format.Disk.DISK_TYPE_HDD, 'NA', 200)
        self.assertIsNone (d.marker)


#    def __init__ (self, path, os_disk, index_disk, disk_type, interface, capacity):



        self.assertFalse (fstab.backup_if_altered ('tests/ExtendedFstabTest.unit.updated'))
        fstab.remove_mount_point_by_uuid ('e1b56c44-dddd-4e4f-9bd5-73ddddddd707')
        fstab.add_mount_point ('# This is a comment')
        self.assertTrue (fstab.backup_if_altered ('tests/ExtendedFstabTest.unit.updated'))

'''
