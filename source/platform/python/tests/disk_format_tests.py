#!/usr/bin/env python

# Unit tests for disk_format.py

import unittest
import mock
import disk_format
import copy
import subprocess


''' extendedFstab class unit tests -- these could use improvements '''
class extendedFstabTest (unittest.TestCase):

    def testFstabInit (self):
        fstab = disk_format.extendedFstab()
        self.assertFalse (fstab.altered)

    def testFstabAdd (self):
        fstab = disk_format.extendedFstab()
        fstab.read ('test_data/ExtendedFstabTest.unit')
        fstab.dump_fstab()
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


''' BaseDisk unit tests '''
class BaseDiskTest (unittest.TestCase):

    def setUp (self):
        self.instance = disk_format.BaseDisk()

    @mock.patch ('disk_format.subprocess.Popen')
    def test_get_uuid (self, mock_popen):

        process_mock = mock.Mock ()
        attrs = {'returncode': 0}
        process_mock.configure (**attrs)

        mock_popen.return_value = process_mock

        fake_path = '/dev/foo'

        mock_args = copy.deepcopy (self.instance.GET_UUID_COMMAND)
        mock_args.append (fake_path)

        self.instance.get_uuid (fake_path)

        mock_popen.assert_called_once_with (mock_args, stdout=-1)


''' class Base unit tests '''
class BaseTest (unittest.TestCase):

    def setUp (self):
        self.instance = disk_format.Base()

    def test_system_exit (self):
        with self.assertRaises (SystemExit) as cm:
            self.instance.system_exit("Unit Test")
        self.assertEqual (cm.exception.code, 8)

    @mock.patch ('disk_format.subprocess.call')
    def test_call_subproc (self, mock_call):
        process_mock = mock.Mock()
        attrs = {'returncode': 0}
        process_mock.configure (**attrs)
        mock_call.return_value = process_mock
        call_list = ['mocked_subprocess_call', '--arg', 'dummy']
        #with self.assertRaises (SystemExit) as cm:
        with self.assertRaises (SystemExit):
            self.instance.call_subproc (call_list)
        mock_call.assert_called_with (call_list, stderr=None, stdout=None)

    def test_dbg_print (self):
        global global_debug_on
        disk_format.global_debug_on = True
        self.instance.dbg_print ("Unit testing")

    def test_dbg_print_list (self):
        global global_debug_on
        disk_format.global_debug_on = True
        self.instance.dbg_print_list (['Unit', 'testing'])


''' Basic Disk class unit tests '''
class DiskTest (unittest.TestCase):

    TEST_PATH = "/dev/foo"
    TEST_CAPACITY = "3006"

    def setUp (self):
        self.instance = disk_format.Disk (DiskTest.TEST_PATH, False, False, disk_format.Disk.DISK_TYPE_HDD, 'NA', DiskTest.TEST_CAPACITY)

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
        d = disk_format.Disk (DiskTest.TEST_PATH, True, True, disk_format.Disk.DISK_TYPE_HDD, 'NA', 200)
        self.assertEqual (DiskTest.TEST_PATH, d.get_path())

    def testDiskgetType (self):
        d = disk_format.Disk (DiskTest.TEST_PATH, True, True, disk_format.Disk.DISK_TYPE_HDD, 'NA', 200)
        self.assertEqual (disk_format.Disk.DISK_TYPE_HDD, d.get_type())

    def testDiskgetCapacity (self):
        d = disk_format.Disk (DiskTest.TEST_PATH, True, True, disk_format.Disk.DISK_TYPE_HDD, 'NA', DiskTest.TEST_CAPACITY)
        self.assertEqual (DiskTest.TEST_CAPACITY, d.get_capacity())

    def testDiskgetIndex (self):
        d = disk_format.Disk (DiskTest.TEST_PATH, True, True, disk_format.Disk.DISK_TYPE_HDD, 'NA', DiskTest.TEST_CAPACITY)
        self.assertTrue (d.get_index())

    def testDiskgetIndex2 (self):
        d = disk_format.Disk (DiskTest.TEST_PATH, True, False, disk_format.Disk.DISK_TYPE_HDD, 'NA', DiskTest.TEST_CAPACITY)
        self.assertFalse (d.get_index())

    def testDiskgetOsUsage (self):
        d = disk_format.Disk (DiskTest.TEST_PATH, True, True, disk_format.Disk.DISK_TYPE_HDD, 'NA', DiskTest.TEST_CAPACITY)
        self.assertTrue (d.get_os_usage())

    def testDiskgetOsUsage2 (self):
        d = disk_format.Disk (DiskTest.TEST_PATH, False, False, disk_format.Disk.DISK_TYPE_HDD, 'NA', DiskTest.TEST_CAPACITY)
        self.assertFalse (d.get_os_usage())

#    def testDiskSmFlag (self):
#        d = disk_format.Disk (DiskTest.TEST_PATH, False, False, disk_format.Disk.DISK_TYPE_HDD, 'NA', DiskTest.TEST_CAPACITY)
#        self.assertFalse (d.sm_flag)
#        d.set_sm_flag()
#        self.assertTrue (d.sm_flag)

    def testDiskDmFlag (self):
        d = disk_format.Disk (DiskTest.TEST_PATH, False, False, disk_format.Disk.DISK_TYPE_HDD, 'NA', DiskTest.TEST_CAPACITY)
        self.assertFalse (d.dm_flag)
        d.set_dm_flag()
        self.assertTrue (d.dm_flag)

    def testDiskFormattedFlag (self):
        d = disk_format.Disk (DiskTest.TEST_PATH, False, False, disk_format.Disk.DISK_TYPE_HDD, 'NA', DiskTest.TEST_CAPACITY)
        self.assertFalse(d.formatted)
        d.set_formatted()
        self.assertTrue (d.formatted)

    def testDiskCheckForMarkerOS (self):
        d = disk_format.Disk (DiskTest.TEST_PATH, True, True, disk_format.Disk.DISK_TYPE_HDD, 'NA', 200)
        self.assertFalse (d.check_for_fds())

    def testDiskCheckForMarkerPresent (self):
        d = disk_format.Disk (DiskTest.TEST_PATH, False, True, disk_format.Disk.DISK_TYPE_HDD, 'NA', 200)
        d.marker = disk_format.DISK_MARKER
        self.assertTrue (d.check_for_fds())

    def testDiskCheckForMarkerNotPresent (self):
        d = disk_format.Disk (DiskTest.TEST_PATH, False, True, disk_format.Disk.DISK_TYPE_HDD, 'NA', 200)
        self.assertFalse (d.check_for_fds())

    def testDiskPartionOSDrive (self):
        d = disk_format.Disk (DiskTest.TEST_PATH, True, True, disk_format.Disk.DISK_TYPE_HDD, 'NA', DiskTest.TEST_CAPACITY)
        self.assertFalse (d.partition (1, 1))

    @mock.patch ('disk_format.Disk.call_subproc')
    def testDiskPartitionDataDisk (self, mock_call_subproc):
        d = disk_format.Disk (DiskTest.TEST_PATH, False, False, disk_format.Disk.DISK_TYPE_HDD, 'NA', DiskTest.TEST_CAPACITY)

        process_mock = mock.Mock ()
        attrs = {'returncode': 0}
        process_mock.configure (attrs)

        mock_call_subproc.return_value = process_mock

        fake_path = '/dev/foo'

        mock_args_mklabel = self.instance.PARTED_MKLABEL_PART_1 + fake_path.split() + self.instance.PARTED_MKLABEL_PART_2

        mock_args_mkpart_sb = disk_format.Disk.PARTED_MKPART_SUPERBLOCK_1 + fake_path.split() + disk_format.Disk.PARTED_MKPART_SUPERBLOCK_2 + disk_format.PARTITION_TYPE.split() + (str (disk_format.PARTITION_START_MB) + 'MB').split() + (str (disk_format.PARTITION_START_MB + disk_format.FDS_SUPERBLOCK_SIZE_IN_MB) + 'MB').split() + disk_format.Disk.PARTED_MKPART_SUPERBLOCK_3

        mock_args_mkpart_data = disk_format.Disk.PARTED_MKPART_DATA_1 + fake_path.split() + disk_format.Disk.PARTED_MKPART_DATA_2 + (str (disk_format.PARTITION_START_MB + disk_format.FDS_SUPERBLOCK_SIZE_IN_MB) + 'MB').split() + disk_format.Disk.PARTED_MKPART_DATA_3

        expected = [mock.call(mock_args_mklabel), mock.call(mock_args_mkpart_sb), mock.call (mock_args_mkpart_data)]

        d.partition (1,1)

        assert mock_call_subproc.call_args_list == expected
        assert 3 == mock_call_subproc.call_count


    @mock.patch ('disk_format.Disk.call_subproc')
    def testDiskPartitionIndexNoType (self, mock_call_subproc):
        d = disk_format.Disk (DiskTest.TEST_PATH, False, True, disk_format.Disk.DISK_TYPE_HDD, 'NA', DiskTest.TEST_CAPACITY)

        process_mock = mock.Mock ()
        attrs = {'returncode': 0}
        process_mock.configure (attrs)

        mock_call_subproc.return_value = process_mock

        fake_path = '/dev/foo'

        mock_args_mklabel = self.instance.PARTED_MKLABEL_PART_1 + fake_path.split() + self.instance.PARTED_MKLABEL_PART_2

        mock_args_mkpart_sb = disk_format.Disk.PARTED_MKPART_SUPERBLOCK_1 + fake_path.split() + disk_format.Disk.PARTED_MKPART_SUPERBLOCK_2 + disk_format.PARTITION_TYPE.split() + (str (disk_format.PARTITION_START_MB) + 'MB').split() + (str (disk_format.PARTITION_START_MB + disk_format.FDS_SUPERBLOCK_SIZE_IN_MB) + 'MB').split() + disk_format.Disk.PARTED_MKPART_SUPERBLOCK_3

        expected = [mock.call(mock_args_mklabel), mock.call(mock_args_mkpart_sb)]

        with self.assertRaises (SystemExit):
            d.partition (1,1)

        assert mock_call_subproc.call_args_list == expected
        assert 2 == mock_call_subproc.call_count


#    @mock.patch ('disk_format.Disk.call_subproc')
#    def testDiskPartitionSMIndex (self, mock_call_subproc):
#        d = disk_format.Disk (DiskTest.TEST_PATH, False, True, disk_format.Disk.DISK_TYPE_HDD, 'NA', DiskTest.TEST_CAPACITY)
#        d.set_sm_flag()
#
#        sm_index_size = 4
#
#        process_mock = mock.Mock ()
#        attrs = {'returncode': 0}
#        process_mock.configure (attrs)
#
#        mock_call_subproc.return_value = process_mock
#
#        fake_path = '/dev/foo'
#
#        mock_args_mklabel = self.instance.PARTED_MKLABEL_PART_1 + fake_path.split() + self.instance.PARTED_MKLABEL_PART_2
#
#        mock_args_mkpart_sb = disk_format.Disk.PARTED_MKPART_SUPERBLOCK_1 + fake_path.split() + disk_format.Disk.PARTED_MKPART_SUPERBLOCK_2 + disk_format.PARTITION_TYPE.split() + (str (disk_format.PARTITION_START_MB) + 'MB').split() + (str (disk_format.PARTITION_START_MB + disk_format.FDS_SUPERBLOCK_SIZE_IN_MB) + 'MB').split() + disk_format.Disk.PARTED_MKPART_SUPERBLOCK_3
#
#        mock_args_mkpart_index = disk_format.Disk.PARTED_MKPART_INDEX_1  + fake_path.split() + disk_format.Disk.PARTED_MKPART_INDEX_2 + disk_format.PARTITION_TYPE.split() + (str (disk_format.PARTITION_START_MB + disk_format.FDS_SUPERBLOCK_SIZE_IN_MB) + 'MB').split() + (str (disk_format.PARTITION_START_MB + disk_format.FDS_SUPERBLOCK_SIZE_IN_MB + sm_index_size) + 'MB').split()
#
#        mock_args_mkpart_data = disk_format.Disk.PARTED_MKPART_DATA_1 + fake_path.split() + disk_format.Disk.PARTED_MKPART_DATA_2 +  (str (disk_format.PARTITION_START_MB + disk_format.FDS_SUPERBLOCK_SIZE_IN_MB + sm_index_size) + 'MB').split() + disk_format.Disk.PARTED_MKPART_DATA_3
#
#        expected = [mock.call (mock_args_mklabel), mock.call (mock_args_mkpart_sb), mock.call (mock_args_mkpart_index), mock.call (mock_args_mkpart_data)]
#
#        d.partition (1,sm_index_size)
#
#        assert mock_call_subproc.call_args_list == expected
#        assert 4 == mock_call_subproc.call_count



    @mock.patch ('disk_format.Disk.call_subproc')
    def testDiskPartitionDriveWBadPartitions (self, mock_call_subproc):
        d = disk_format.Disk (DiskTest.TEST_PATH, False, False, disk_format.Disk.DISK_TYPE_HDD, 'NA', DiskTest.TEST_CAPACITY)
        partname = DiskTest.TEST_PATH + "1"
        badpartfile = open(partname, 'w').close()
        call_list = ['ls', partname]
        res = subprocess.call (call_list, stdout = None)
        assert (res == 0)
        d.partition(1,1)
        res = subprocess.call (call_list, stdout = None)
        assert (res == 2)

    @mock.patch ('disk_format.Disk.call_subproc')
    def testDiskPartitionDMIndex (self, mock_call_subproc):
        d = disk_format.Disk (DiskTest.TEST_PATH, False, True, disk_format.Disk.DISK_TYPE_HDD, 'NA', DiskTest.TEST_CAPACITY)
        d.set_dm_flag()

        dm_index_size = 6

        process_mock = mock.Mock ()
        attrs = {'returncode': 0}
        process_mock.configure (attrs)

        mock_call_subproc.return_value = process_mock

        fake_path = '/dev/foo'

        mock_args_mklabel = self.instance.PARTED_MKLABEL_PART_1 + fake_path.split() + self.instance.PARTED_MKLABEL_PART_2

        mock_args_mkpart_sb = disk_format.Disk.PARTED_MKPART_SUPERBLOCK_1 + fake_path.split() + disk_format.Disk.PARTED_MKPART_SUPERBLOCK_2 + disk_format.PARTITION_TYPE.split() + (str (disk_format.PARTITION_START_MB) + 'MB').split() + (str (disk_format.PARTITION_START_MB + disk_format.FDS_SUPERBLOCK_SIZE_IN_MB) + 'MB').split() + disk_format.Disk.PARTED_MKPART_SUPERBLOCK_3

        mock_args_mkpart_index = disk_format.Disk.PARTED_MKPART_INDEX_1  + fake_path.split() + disk_format.Disk.PARTED_MKPART_INDEX_2 + disk_format.PARTITION_TYPE.split() + (str (disk_format.PARTITION_START_MB + disk_format.FDS_SUPERBLOCK_SIZE_IN_MB) + 'MB').split() + (str (disk_format.PARTITION_START_MB + disk_format.FDS_SUPERBLOCK_SIZE_IN_MB + dm_index_size) + 'MB').split()

        mock_args_mkpart_data = disk_format.Disk.PARTED_MKPART_DATA_1 + fake_path.split() + disk_format.Disk.PARTED_MKPART_DATA_2 +  (str (disk_format.PARTITION_START_MB + disk_format.FDS_SUPERBLOCK_SIZE_IN_MB + dm_index_size) + 'MB').split() + disk_format.Disk.PARTED_MKPART_DATA_3

        expected = [mock.call (mock_args_mklabel), mock.call (mock_args_mkpart_sb), mock.call (mock_args_mkpart_index), mock.call (mock_args_mkpart_data)]

        d.partition (dm_index_size, 1)

        assert mock_call_subproc.call_args_list == expected
        assert 4 == mock_call_subproc.call_count


    @mock.patch ('__builtin__.open', mock.mock_open (read_data=disk_format.DISK_MARKER), create=True)
    def testDiskLoadDiskMarker (self):
        d = disk_format.Disk (DiskTest.TEST_PATH, False, False, disk_format.Disk.DISK_TYPE_HDD, 'NA', DiskTest.TEST_CAPACITY)
        assert d.marker == disk_format.DISK_MARKER

    @mock.patch ('__builtin__.open', mock.mock_open (read_data="20480"), create=True)
    def testDiskVerifySystemDiskPartitionSizeGood (self):
        d = disk_format.Disk (DiskTest.TEST_PATH, False, False, disk_format.Disk.DISK_TYPE_HDD, 'NA', DiskTest.TEST_CAPACITY)
        d.verifySystemDiskPartitionSize()

    @mock.patch ('__builtin__.open', mock.mock_open (read_data="44444"), create=True)
    def testDiskVerifySystemDiskPartitionSizeFail (self):
        d = disk_format.Disk (DiskTest.TEST_PATH, False, False, disk_format.Disk.DISK_TYPE_HDD, 'NA', DiskTest.TEST_CAPACITY)
        with self.assertRaises (SystemExit) as cm:
            d.verifySystemDiskPartitionSize()
        self.assertEqual (cm.exception.code, 8)

    @mock.patch ('__builtin__.open', mock.mock_open (read_data=disk_format.DISK_MARKER), create=True)
    @mock.patch ('disk_format.Disk.call_subproc')
    @mock.patch ('disk_format.subprocess.Popen')
    def testDiskFormatOSDrive (self, mock_popen, mock_call_subproc):
        process_mock = mock.Mock ()
        attrs = {'returncode': 0}
        process_mock.stdout.read.return_value = '33333'
        process_mock.configure (**attrs)

        mock_popen.return_value = process_mock
        mock_call_subproc.return_value = process_mock

        d = disk_format.Disk (DiskTest.TEST_PATH, True, True, disk_format.Disk.DISK_TYPE_HDD, 'NA', DiskTest.TEST_CAPACITY)
        d.format()

        expected = []

        assert mock_call_subproc.call_args_list == expected
        assert 0 == mock_call_subproc.call_count
        self.assertFalse (d.format ())

    @mock.patch ('__builtin__.open', mock.mock_open (read_data=disk_format.DISK_MARKER), create=True)
    @mock.patch ('disk_format.Disk.call_subproc')
    @mock.patch ('disk_format.subprocess.Popen')
    def testDiskFormatDataDrive (self, mock_popen, mock_call_subproc):
        process_mock = mock.Mock ()
        attrs = {'returncode': 0}
        process_mock.stdout.read.return_value = '33333'
        process_mock.configure (**attrs)

        mock_popen.return_value = process_mock
        mock_call_subproc.return_value = process_mock

        d = disk_format.Disk (DiskTest.TEST_PATH, False, False, disk_format.Disk.DISK_TYPE_HDD, 'NA', DiskTest.TEST_CAPACITY)
        d.format()

        mock_args_format = disk_format.Disk.MKFS_PART_1 + (DiskTest.TEST_PATH + '2').split() + disk_format.Disk.MKFS_PART_2

        expected = [mock.call (mock_args_format)]

        assert mock_call_subproc.call_args_list == expected
        assert 1 == mock_call_subproc.call_count


    @mock.patch ('__builtin__.open', mock.mock_open (read_data=disk_format.DISK_MARKER), create=True)
    @mock.patch ('disk_format.Disk.call_subproc')
    @mock.patch ('disk_format.subprocess.Popen')
    def testDiskFormatIndexAndDataDrive (self, mock_popen, mock_call_subproc):
        process_mock = mock.Mock ()
        attrs = {'returncode': 0}
        process_mock.stdout.read.return_value = '33333'
        process_mock.configure (**attrs)

        mock_popen.return_value = process_mock
        mock_call_subproc.return_value = process_mock

        d = disk_format.Disk (DiskTest.TEST_PATH, False, True, disk_format.Disk.DISK_TYPE_HDD, 'NA', DiskTest.TEST_CAPACITY)
        d.format()

        expected = []

        for part in ['2', '3']:
            mock_args_format = disk_format.Disk.MKFS_PART_1 + (DiskTest.TEST_PATH + part).split() + disk_format.Disk.MKFS_PART_2
            expected.append(mock.call (mock_args_format))

        assert mock_call_subproc.call_args_list == expected
        assert 2 == mock_call_subproc.call_count



#class testRaidDevice (unittest.TestCase):
#
#    def setUp (self):
#        self.raid = disk_format.RaidDevice()
#
#    @mock.patch ('disk_format.RaidDevice.call_subproc')
#    def testRaidDeviceResetRaidComponents (self, mock_subproc):
#        process_mock = mock.Mock ()
#        attrs = {'returncode': 0}
#        process_mock.configure (**attrs)
#
#        mock_subproc.return_value = process_mock
#
#        raid_partitions = ['/dev/idxa2', '/dev/idxb2']
#        md_device = '/dev/mdNNN'
#
#        mock_args_stop = disk_format.RaidDevice.MDADM_STOP_RAID_1 + md_device.split()
#        mock_args_zero = disk_format.RaidDevice.MDADM_ZERO_SB_1
#
#        for part in raid_partitions:
#            mock_args_zero.append (part)
#
#        self.raid.reset_raid_components(md_device, raid_partitions)
#
#        expected = [mock.call (mock_args_stop), mock.call (mock_args_zero)]
#
#        assert mock_subproc.call_args_list == expected
#        assert 2 == mock_subproc.call_count
#
#    @mock.patch ('disk_format.RaidDevice.get_uuid')
#    @mock.patch ('disk_format.RaidDevice.call_subproc')
#    @mock.patch ('disk_format.DiskUtils.find_mounts')
#    @mock.patch ('disk_format.os.path')
#    @mock.patch ('disk_format.subprocess.Popen')
#    def testRaidDeviceCleanupRaidIfUsedPresent (self, mock_popen, mock_os_path, mock_find_mounts, mock_subproc, mock_uuid):
#
#        popen_mock = mock.Mock()
#        attrs = {'returncode': 0}
#        popen_mock.configure (**attrs)
#        popen_mock.stdout = ['/dev/idxa2', '/dev/idxb2']
#        mock_popen.return_value = popen_mock
#
#        md_device = '/dev/md0'
#
#        mock_os_path.exists.return_value = True
#
#        find_mounts_mock = mock.Mock (return_value = ['/dev/mdNNN'])
#        mock_find_mounts.return_value = find_mounts_mock.return_value
#
#        subproc_mock = mock.Mock()
#        attrs = {'returncode': 0}
#        subproc_mock.configure (**attrs)
#        mock_subproc.return_value = subproc_mock.return_value
#
#        uuid_mock = mock.Mock (return_value = 'bazmoonar')
#        mock_uuid.return_value = uuid_mock.return_value
#
#        fstab = disk_format.extendedFstab()
#        fstab.read ('test_data/ExtendedFstabTest.unit')
#        raid_partitions = ['/dev/idxa2', '/dev/idxb2']
#        disk_utils = disk_format.DiskUtils()
#
#        mock_args_umount = disk_format.RaidDevice.UMOUNT_COMMAND_1 + mock_find_mounts.return_value
#        mock_args_stop = disk_format.RaidDevice.MDADM_STOP_RAID_1 + md_device.split()
#        mock_args_zero = disk_format.RaidDevice.MDADM_ZERO_SB_1
#
#        expected = [mock.call (mock_args_umount), mock.call (mock_args_stop), mock.call (mock_args_zero)]
#
#        self.raid.cleanup_raid_if_in_use (raid_partitions, fstab, disk_utils)
#
#
#        assert mock_subproc.call_args_list == expected
#        assert 3 == mock_subproc.call_count
#
#    @mock.patch ('disk_format.os.path')
#    def testRaidDeviceCleanupRaidIfUsedNotPresent (self, mock_os_path):
#
#        mock_os_path.exists.return_value = False
#
#        fstab = disk_format.extendedFstab()
#        fstab.read ('test_data/ExtendedFstabTest.unit')
#        raid_partitions = ['/dev/idxa2', '/dev/idxb2']
#        disk_utils = disk_format.DiskUtils()
#
#        value = self.raid.cleanup_raid_if_in_use(raid_partitions, fstab, disk_utils)
#
#        assert False == value
#
#    def testRaidDeviceCreateIndexRaidShortList (self):
#        raid_partitions = ['/dev/idxa2']
#        uuid = self.raid.create_index_raid (raid_partitions)
#        assert uuid is None
#
#    @mock.patch ('disk_format.os.path')
#    def testRaidDeviceCreateIndexRaidNoDevAvailable (self, mock_os_path):
#        raid_partitions = ['/dev/idxa2', '/dev/idxb2']
#
#        mock_os_path.exists.return_value = True
#
#        with self.assertRaises (SystemExit) as cm:
#            self.raid.create_index_raid (raid_partitions)
#        self.assertEqual (cm.exception.code, 8)
#
#
#    @mock.patch ('disk_format.RaidDevice.get_uuid')
#    @mock.patch ('disk_format.RaidDevice.call_subproc')
#    @mock.patch ('disk_format.os.path')
#    def testRaidDeviceCreateIndexRaidWorks (self, mock_os_path, mock_subproc, mock_uuid):
#        raid_partitions = ['/dev/idxa2', '/dev/idxb2']
#        raid_device = '/dev/md0'
#
#        mock_os_path.exists.return_value = False
#
#        subproc_mock = mock.Mock()
#        attrs = {'returncode': 0}
#        subproc_mock.configure (**attrs)
#        mock_subproc.return_value = subproc_mock.return_value
#
#        uuid_mock = mock.Mock (return_value = 'biznoolar')
#        mock_uuid.return_value = uuid_mock.return_value
#
#        uuid = self.raid.create_index_raid (raid_partitions)
#
#        mock_args_md_create = disk_format.RaidDevice.MDADM_CREATE_1 + raid_device.split() + disk_format.RaidDevice.MDADM_CREATE_2 + raid_partitions
#        mock_args_format = disk_format.Disk.MKFS_PART_1 + raid_device.split() + disk_format.Disk.MKFS_PART_2
#
#        expected = [mock.call (mock_args_md_create), mock.call (mock_args_format)]
#
#        assert mock_subproc.call_args_list == expected
#        assert 2 == mock_subproc.call_count



class testDiskUtils (unittest.TestCase):

    def setUp (self):

        self.du = disk_format.DiskUtils()


    # @mock.patch ('__builtin__.open', mock.mock_open (read_data="/dev/sda"), create=True)
    def testFindMounts (self):
        test_mount_point = '/foo/bar'
        fake_mounts = '\n'.join(['/dev/sda1 ' + test_mount_point, '/dev/sdb1 /moo/more'])

        with mock.patch ('__builtin__.open', mock.mock_open (read_data=fake_mounts), create=True) as mo:
            # from http://bash-shell.net/blog/2014/feb/27/file-iteration-python-mock/
            # mock_open doesn't properly handle iterating over the open file with for line in file:
            # but if we set the return value like this, it works.
            mo.return_value.__iter__.return_value = fake_mounts.splitlines()
            mount_points = self.du.find_mounts ("sda")

        assert test_mount_point == mount_points[0]

    def testIsMounted (self):
        dev = self.du.is_mounted("/")
        assert dev # the root partition must be mounted
        dev = self.du.is_mounted("blah")
        assert not dev # bad mount point

    def my_find_fs (self, param):
        if param == 'UUID=cazzoomar':
            return '/dev/sda'
        return None

    def testFindDeviceByUUIDStr (self):
        dev = self.du.find_device_by_uuid_str("blah")
        assert not dev
        call_list = ['blkid', '-o', 'device']
        output = subprocess.Popen (call_list, stdout=subprocess.PIPE).stdout
        for line in output:
            dev = line.strip ('\r\n')
            uuid = self.du.get_uuid(dev)
            if uuid:
                retdev = self.du.find_device_by_uuid_str("UUID="+uuid)
                assert retdev == dev

    @mock.patch ('disk_format.DiskUtils.call_subproc')
    def testCleanUpMounted (self, mock_subproc):
        target_path = '/moo/more'
        linked_partition = '/dev/pb2'
        partitions = ['/dev/pa2', linked_partition]
        fake_mounts = '\n'.join(['/dev/sda1 /moo/less', linked_partition + ' ' + target_path])

        fstab = disk_format.extendedFstab()
        fstab.read ('test_data/ExtendedFstabTest.unit')

        with mock.patch ('__builtin__.open', mock.mock_open (read_data=fake_mounts), create=True) as mo:
            mo.return_value.__iter__.return_value = fake_mounts.splitlines()
            self.du.cleanup_mounted_file_systems (fstab, partitions)

        umount_cmd_args = disk_format.DiskUtils.UMOUNT_COMMAND_1 + target_path.split()

        expected = [mock.call(umount_cmd_args)]

        assert mock_subproc.call_args_list == expected
        assert 1 == mock_subproc.call_count


    def testFindDiskTypeFound (self):
        disk_list = []
        d = disk_format.Disk (DiskTest.TEST_PATH, False, False, disk_format.Disk.DISK_TYPE_HDD, 'NA', DiskTest.TEST_CAPACITY)
        disk_list.append (d)

        assert self.du.find_disk_type (disk_list, DiskTest.TEST_PATH) == disk_format.Disk.DISK_TYPE_HDD


    def testFindDiskTypeNotFound (self):
        disk_list = []

        with self.assertRaises (SystemExit) as cm:
            self.du.find_disk_type (disk_list, DiskTest.TEST_PATH)
        self.assertEqual (cm.exception.code, 8)


class testDiskManager (unittest.TestCase):

    DISK_DATA="#ants\n/dev/os   1       0          HDD         NA    32"

    @mock.patch ('disk_format.os')
    def setUp (self, mock_os):
        mock_os.getuid.return_value = 0
        self.manager = disk_format.DiskManager()
        self.manager.process_command_line(['--map', 'test_data/disk-config.conf'])


    def testDiskManagerLoadDiskConfigInvalid (self):
        self.manager.disk_config_file = 'test_data/disk_config.invalid'
        with self.assertRaises (SystemExit) as cm:
            self.manager.load_disk_config_file()

        assert 4 == len (self.manager.disk_list)
        self.assertEqual (cm.exception.code, 8)


    @mock.patch ('disk_format.os')
    def testDiskManagerNoCliArgs (self, mock_os):
        mock_os.getuid.return_value = 0
        self.manager.process_command_line()      # override the setUp version of the command line

        assert 20 == len (self.manager.options.disk_config_file)


    def testDiskManagerLoadDiskConfig (self):
        self.manager.disk_config_file = 'test_data/disk_config'
        self.manager.load_disk_config_file()
        with self.assertRaises (SystemExit) as cm:
            self.manager.disk_report()

        assert 6 == len (self.manager.disk_list)
        self.assertEqual (cm.exception.code, 8)

    @mock.patch ('disk_format.Disk.verifySystemDiskPartitionSize')
    @mock.patch ('disk_format.Disk.format')
    @mock.patch ('disk_format.Disk.partition')
    def testDiskManagerFormatExisting (self, mock_verifySystemDiskPartitionSize, mock_format, mock_partition):
        self.manager.disk_config_file = 'test_data/disk_config'
        self.manager.load_disk_config_file()

        self.manager.disk_list[5].marker = disk_format.DISK_MARKER
        self.manager.find_formatted_disks()
        i = 0
        for disk in self.manager.disk_list:
            if disk.formatted == True:
                assert i == 5
            i = i + 1
        assert self.manager.disk_list[5].formatted == True

        self.manager.global_debug_on = True
        self.manager.partition_and_format_disks()
        assert 3 == mock_format.call_count # 6 - 2(os) -1(formatted)

    def testDiskManagerBuildPartitionListsWExisting (self):
        self.manager.disk_config_file = 'test_data/disk_config'
        self.manager.load_disk_config_file()

        self.manager.disk_list[5].marker = disk_format.DISK_MARKER
        self.manager.find_formatted_disks()
        self.manager.build_partition_lists()
        num_parts = len(self.manager.umount_list)
        assert 4 == num_parts # 4 = 3(SM) + 1(DM)

    def testDiskManagerCalcCapacities (self):
        self.manager.disk_config_file = 'test_data/disk_config'
        self.manager.load_disk_config_file()

        self.manager.calculate_capacities()

        assert 142 == self.manager.dm_index_MB
#        assert 2668 == self.manager.sm_index_MB

    def testDiskManagerCalcCapacitiesNormalCapacity (self):
        self.manager.disk_config_file = 'test_data/disk_config.normal'
        self.manager.load_disk_config_file()
        self.manager.calculate_capacities()

        assert 5216 == self.manager.dm_index_MB
#        assert 98278 == self.manager.sm_index_MB

    def testDiskManagerBuildPartLists (self):
        self.manager.disk_config_file = 'test_data/disk_config.normal'
        self.manager.load_disk_config_file()

        self.manager.build_partition_lists()

        assert ['/dev/sdzx2'] == self.manager.dm_index_partition_list
#        assert ['/dev/sdzw2', '/dev/sdzv2'] == self.manager.sm_index_partition_list
        assert ['/dev/sdzx3', '/dev/sdzw3', '/dev/sdzv3', '/dev/sdzu2'] == self.manager.data_partition_list
        assert ['/dev/sdzx3', '/dev/sdzw3', '/dev/sdzv3', '/dev/sdzu2', '/dev/sdzx2'] == self.manager.umount_list


    @mock.patch ('disk_format.Disk.format')
    @mock.patch ('disk_format.Disk.partition')
    def testDiskManagerPartAndFormatDisksNoOS (self, mock_partition, mock_format):

        process_mock = mock.Mock ()
        attrs = {'returncode': 0}
        process_mock.configure (**attrs)

        mock_partition.return_value = process_mock
        mock_format.return_value = process_mock

        self.manager.disk_config_file = 'test_data/disk_config.no-os-drives'
        self.manager.load_disk_config_file()

        self.manager.build_partition_lists()
        self.manager.partition_and_format_disks()

        assert 4 == mock_partition.call_count
        assert 4 == mock_format.call_count

    @mock.patch ('disk_format.Disk.format')
    @mock.patch ('disk_format.Disk.partition')
    def testDiskManagerPartAndFormatDisksOSPartBadPath (self, mock_partition, mock_format):

        process_mock = mock.Mock ()
        attrs = {'returncode': 0}
        process_mock.configure (**attrs)

        mock_partition.return_value = process_mock
        mock_format.return_value = process_mock

        self.manager.disk_config_file = 'test_data/disk_config.normal'
        self.manager.load_disk_config_file()

        self.manager.build_partition_lists()

        with self.assertRaises (SystemExit) as cm:
           self.manager.partition_and_format_disks()
        self.assertEqual (cm.exception.code, 8)


#    @mock.patch ('disk_format.RaidDevice.get_uuid')
#    @mock.patch ('disk_format.RaidDevice.create_index_raid')
#    def testDiskManagerAddSmMountPointWithRaid (self, mock_create_index, mock_uuid):
#
#        process_mock = mock.Mock ()
#        attrs = {'returncode': 0}
#        process_mock.configure (**attrs)
#
#        uuid_mock = mock.Mock (return_value = 'bazzoofar')
#        mock_uuid.return_value = uuid_mock.return_value
#
#        self.manager.disk_config_file = 'test_data/disk_config.normal'
#        self.manager.load_disk_config_file()
#
#        self.manager.build_partition_lists()
#
##        self.manager.raid_manager = disk_format.RaidDevice()
#        self.manager.fstab = disk_format.extendedFstab()
#        self.manager.fstab.read ('test_data/ExtendedFstabTest.unit')
#
#        self.manager.add_sm_mount_point()
#
#        self.assertTrue (self.manager.fstab.altered)
#
#
##    @mock.patch ('disk_format.RaidDevice.get_uuid')
##    def testDiskManagerAddSmMountPointWithOutRaid (self, mock_uuid):
#    def testDiskManagerAddSmMountPointWithOutRaid (self):
#
#        process_mock = mock.Mock ()
#        attrs = {'returncode': 0}
#        process_mock.configure (**attrs)
#
##        uuid_mock = mock.Mock (return_value = 'bazzoofar')
##        mock_uuid.return_value = uuid_mock.return_value
#
#        self.manager.disk_config_file = 'test_data/disk_config'
#        self.manager.load_disk_config_file()
#
#        self.manager.build_partition_lists()
#
##        self.manager.raid_manager = disk_format.RaidDevice()
#        self.manager.fstab = disk_format.extendedFstab()
#        self.manager.fstab.read ('test_data/ExtendedFstabTest.unit')
#
#        self.manager.add_sm_mount_point()
#
#        self.assertTrue (self.manager.fstab.altered)


#    @mock.patch ('disk_format.RaidDevice.get_uuid')
#    def testDiskManagerAddDmMountPoint (self, mock_uuid):
    def testDiskManagerAddDmMountPoint (self):

#        uuid_mock = mock.Mock (return_value = 'bazzoomar')
#        mock_uuid.return_value = uuid_mock.return_value

        self.manager.disk_config_file = 'test_data/disk_config'
        self.manager.load_disk_config_file()

        self.manager.build_partition_lists()

        self.manager.fstab = disk_format.extendedFstab()
        self.manager.fstab.read ('test_data/ExtendedFstabTest.unit')

        self.manager.add_dm_mount_point()

        self.assertTrue (self.manager.fstab.altered)

    @mock.patch ('disk_format.os')
#    @mock.patch ('disk_format.RaidDevice.get_uuid')
#    def testDiskManagerAddDataMountPoints (self, mock_uuid, mock_os):
    def testDiskManagerAddDataMountPoints (self, mock_os):

        uuid_mock = mock.Mock (return_value = 'bazzoomar')
#        mock_uuid.return_value = uuid_mock.return_value

        mock_os.getuid.return_value = 0

        self.manager.disk_config_file = 'test_data/disk_config.normal'
        self.manager.load_disk_config_file()

        self.manager.build_partition_lists()

        self.manager.fstab = disk_format.extendedFstab()
        self.manager.fstab.read ('test_data/ExtendedFstabTest.unit')

        self.manager.add_data_mount_points()

        self.assertTrue (self.manager.fstab.altered)


#    @mock.patch ('disk_format.RaidDevice.get_uuid')
#    def testDiskManagerProcessReport (self, mock_uuid):
    def testDiskManagerProcessReport (self):
#        uuid_mock = mock.Mock (return_value = 'bazzoomar')
#        mock_uuid.return_value = uuid_mock.return_value

        with self.assertRaises (SystemExit) as cm:
            self.manager.process()
        self.assertEqual (cm.exception.code, 8)


    @mock.patch ('disk_format.DiskManager.partition_and_format_disks')
    @mock.patch ('disk_format.subprocess.Popen')
    @mock.patch ('disk_format.os')
    @mock.patch ('disk_format.DiskUtils.get_uuid')
#    @mock.patch ('disk_format.RaidDevice.get_uuid')
#    def testDiskManagerProcessFormatExisting (self, mock_uuid, mock_uuid_diskutils, mock_os, mock_popen):
    def testDiskManagerProcessFormatExisting (self, mock_uuid_diskutils, mock_os, mock_popen, mock_partition_and_format_disks):
#        mock_uuid.return_value = uuid_mock.return_value

#        uuid_mock = mock.Mock (return_value = 'cazzoomar')
        self.uuid_mock = mock.Mock (return_value = 'cazzoomar')
        mock_uuid_diskutils.return_value = self.uuid_mock.return_value

        mock_os.getuid.return_value = 0

        popen_mock = mock.Mock()
        attrs = {'returncode': 0}
        popen_mock.configure (**attrs)
        popen_mock.stdout = ['/dev/idxa2', '/dev/idxb2']
        mock_popen.return_value = popen_mock

        self.manager.process_command_line(['--format', '--map', 'test_data/disk-config.conf'])

        self.manager.process()

    @mock.patch ('disk_format.os')
#    @mock.patch ('disk_format.RaidDevice.get_uuid')
#    def testDiskManagerProcessDiskFilter (self, mock_uuid, mock_os):
    def testDiskManagerProcessDiskFilter (self, mock_os):
#        uuid_mock = mock.Mock (return_value = 'dazzoomar')
#        mock_uuid.return_value = uuid_mock.return_value

        mock_os.getuid.return_value = 0

        with self.assertRaises (SystemExit) as cm:
            self.manager.process_command_line(['--device', '/dev/foo', '--map', 'test_data/disk-config.conf'])
        self.assertEqual (cm.exception.code, 8)


    @mock.patch ('disk_format.os')
#    @mock.patch ('disk_format.RaidDevice.get_uuid')
#    def testDiskManagerProcessFDSPath (self, mock_uuid, mock_os):
    def testDiskManagerProcessFDSPath (self, mock_os):
#        uuid_mock = mock.Mock (return_value = 'fazzoomar')
#        mock_uuid.return_value = uuid_mock.return_value

        mock_os.getuid.return_value = 0

        self.manager.process_command_line(['--fds-root', '/foo'])

        assert '/foo/' + self.manager.options.disk_config_file == self.manager.disk_config_file

    @mock.patch ('disk_format.os')
#    @mock.patch ('disk_format.RaidDevice.get_uuid')
#    def testDiskManagerProcessDiskRootMap (self, mock_uuid, mock_os):
    def testDiskManagerProcessDiskRootMap (self, mock_os):
#        uuid_mock = mock.Mock (return_value = 'dazzoomar')
#        mock_uuid.return_value = uuid_mock.return_value

        mock_os.getuid.return_value = 0

        with self.assertRaises (SystemExit) as cm:
            self.manager.process_command_line (['--fds-root', '/foo', '--map', 'test_data/disk-config.conf'])
        self.assertEqual (cm.exception.code, 8)


    @mock.patch ('disk_format.os')
#    @mock.patch ('disk_format.RaidDevice.get_uuid')
#    def testDiskManagerProcessDiskMap (self, mock_uuid, mock_os):
    def testDiskManagerProcessDiskMap (self, mock_os):
#        uuid_mock = mock.Mock (return_value = 'dazzoomar')
#        mock_uuid.return_value = uuid_mock.return_value

        mock_os.getuid.return_value = 0

        disk_map = 'test_data/disk-config.conf'

        self.manager.process_command_line (['--map', disk_map])

        assert disk_map == self.manager.disk_config_file

    #@mock.patch ('disk_format.fstab')
    @mock.patch ('disk_format.subprocess.Popen')
    @mock.patch ('disk_format.os')
    @mock.patch ('disk_format.Disk.verifySystemDiskPartitionSize')
    @mock.patch ('disk_format.Disk.format')
    @mock.patch ('disk_format.Disk.partition')
    @mock.patch ('disk_format.DiskUtils.get_uuid')
#    @mock.patch ('disk_format.RaidDevice.get_uuid')
#    def testDiskManagerProcessFormatReset (self, mock_uuid, mock_uuid_diskutils, mock_part_and_format, mock_os, mock_popen):
    def testDiskManagerProcessFormatReset (self, mock_uuid_diskutils, mock_verifySystemDiskPartitionSize, mock_format, mock_partition, mock_os, mock_popen):
#        uuid_mock = mock.Mock (return_value = 'cazzoomar')
#        mock_uuid.return_value = uuid_mock.return_value
        self.uuid_mock = mock.Mock (return_value = 'cazzoomar')
        mock_uuid_diskutils.return_value = self.uuid_mock.return_value

        mock_os.getuid.return_value = 0

        fstab_mock = mock.Mock (return_value = True)

        popen_mock = mock.Mock()
        attrs = {'returncode': 0}
        popen_mock.configure (**attrs)
        popen_mock.stdout = ['/dev/idxa2', '/dev/idxb2']
        mock_popen.return_value = popen_mock

        self.manager.process_command_line(['--format', '--reset', '--fstab', 'test_data/ExtendedFstabTest.unit', '--map', 'test_data/disk-config.conf'])

        self.manager.process()

    def testDiskManagerCleanupFstab(self):
        fstab_file = 'test_data/ExtendedFstabTest.unit'
        self.manager.fstab.read (fstab_file)
        mount_point='/fds/sys-repo'
        uuid='fakeuuid'
        uuid_strs = self.manager.fstab.get_devices_by_mount_point(mount_point)
        assert len(uuid_strs) > 0
        self.manager.cleanup_fstab(uuid, mount_point)
        uuid_strs = self.manager.fstab.get_devices_by_mount_point(mount_point)
        assert len(uuid_strs) == 0

    def my_is_mounted(self, param):
        if param == 'hdd-1':
            return '/dev/sda'
        return None

    @mock.patch ('disk_format.DiskUtils.is_mounted')
    def testGenerateMountPointIndex(self, mock_is_mounted):
        mock_is_mounted.side_effect = self.my_is_mounted
        fstab_file = 'test_data/ExtendedFstabTest.unit'
        self.manager.fstab.read (fstab_file)

        index = 1
        uuid = '1'
        base_name="hdd-"
        index = self.manager.generate_mount_point_index(base_name, index, uuid)
        assert index == 2
