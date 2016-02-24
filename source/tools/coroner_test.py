#!/usr/bin/env python
# Unit tests for coroner script

from coroner import FDSCoroner
import time
import mock
import unittest

class TestFDSCoroner(unittest.TestCase):

    @mock.patch('coroner.os')
    def setUp(self, mock_os):
        mock_os.geteuid.return_value = 0
        # mock_os.path.exists.return_value = False
        # mock_os.makedirs.return_value = True
        # mock_prep_bag.return_value = 'abc123'
        self.bodybag = FDSCoroner()

    @mock.patch('coroner.os.geteuid')
    def test_check_uid(self, mock_os_geteuid):
        """When our UID is not 0 we should raise an exit"""
        mock_os_geteuid.return_value = 1
        self.assertRaises(SystemExit, self.bodybag.check_uid)

    @mock.patch('coroner.socket.gethostname')
    def test_get_bag_name_with_date_noref(self, mock_socket_gethostname):
        self.bodybag.destdir = "/tmp"
        mock_socket_gethostname.return_value = "foohost"
        self.assertEqual(
                self.bodybag.get_bag_name(date="1234"),
                "/tmp/fdscoroner_1234_foohost_noref"
        )

    @mock.patch('coroner.socket.gethostname')
    def test_get_bag_name_without_date_noref(self, mock_socket_gethostname):
        self.bodybag.destdir = "/tmp"
        mock_socket_gethostname.return_value = "foohost"
        self.assertRegexpMatches(
                self.bodybag.get_bag_name(),
                '/fdscoroner_[0-9]+-[0-9]+_foohost_noref'
        )

    @mock.patch('coroner.socket.gethostname')
    def test_get_bag_name_without_date_with_ref(self, mock_socket_gethostname):
        self.bodybag.destdir = "/tmp"
        self.bodybag.refid = "fooref"
        mock_socket_gethostname.return_value = "foohost"
        self.assertRegexpMatches(
                self.bodybag.get_bag_name(),
                '/fdscoroner_[0-9]+-[0-9]+_foohost_fooref'
        )

    @mock.patch('coroner.os')
    def test_make_dir_when_exists(self, mock_os):
        mock_os.path.exists.return_value = True
        self.bodybag.make_dir("test123")
        self.assertFalse(mock_os.makedirs.called, 'Do not create directory if present')

    @mock.patch('coroner.os')
    def test_make_dir_when_not_exists(self, mock_os):
        mock_os.path.exists.return_value = False
        mock_os.makedirs.return_value = True
        self.bodybag.make_dir("test123")
        self.assertTrue(mock_os.makedirs.called, 'Create directory if not present')

    @unittest.skip('not failing as expected')
    @mock.patch('coroner.os')
    def test_make_dir_exception_when_makedirs_fails(self, mock_os):
        mock_os.path.exists.return_value = False
        mock_os.makedirs.return_value = OSError('test123 exists')
        self.bodybag.make_dir("test123")
        self.assertRaises(OSError)

    def test_get_dir_path(self):
        self.bodybag.bodybag = "/foobar"
        self.assertEqual(self.bodybag.get_dir_path("foobaz"), "/foobar/foobaz")

    @mock.patch('coroner.os')
    @mock.patch('coroner.logging')
    def test_prep_bag_when_exists_raises(self, mock_os, mock_logging):
        mock_os.path.exists.return_value = True
        mock_os.makedirs.return_value = True
        with self.assertRaises(SystemExit):
            self.bodybag.prep_bag()

    @mock.patch('coroner.os')
    def test_prep_bag_when_not_exists_calls_makedirs(self, mock_os):
        mock_os.path.exists.return_value = False
        mock_os.makedirs.return_value = True
        self.bodybag.prep_bag()
        self.assertTrue(mock_os.makedirs.called)

    @mock.patch('coroner.os')
    def test_prep_bag_when_not_exists_sets_bodybag(self, mock_os):
        with mock.patch('coroner.FDSCoroner.get_bag_name') as mock_get_bag_name:
            mock_get_bag_name.return_value = "abc"
            mock_os.path.exists.return_value = False
            mock_os.makedirs.return_value = True
            self.bodybag.prep_bag()
            self.assertTrue(self.bodybag.bodybag == 'abc')

    def test_prep_data_dir_calls_make_dir(self):
        with mock.patch('coroner.FDSCoroner.make_dir') as mock_make_dir:
            self.bodybag.prep_data_dir("foobar")
            self.assertTrue(mock_make_dir.called)

    def test_prep_data_dir_calls_make_dir(self):
        dirpath = "/abc123/foobar"
        self.bodybag.get_dir_path = mock.MagicMock(return_value=dirpath)
        self.bodybag.make_dir = mock.MagicMock(return_value=True)
        returnval = self.bodybag.prep_data_dir('foobar')
        self.assertTrue(returnval == dirpath)

    @mock.patch('coroner.logging')
    @mock.patch('coroner.subprocess.call')
    @mock.patch('coroner.FDSCoroner.prep_data_dir')
    @mock.patch('coroner.os.path')
    def test_collect_dir(self, mock_path, mock_prep_data_dir, mock_call, mock_log):
        process_mock = mock.Mock()
        attrs = { 'returncode': 0 }
        process_mock.configure_mock(**attrs)
        mock_call.return_value = process_mock
        mock_path.exists.return_value = True
        mock_path.basename.return_value = "capture_dir"
        mock_path.dirname.return_value = "/capture_dir_parent"
        mock_prep_data_dir.return_value = "/tmp/bag_dir"
        mock_args = ['/bin/tar', '-C', '/capture_dir_parent', '--exclude-from', '/capture_dir_parent/sbin/coroner.excludes', '-chzf', '/tmp/bag_dir/capture_dir.tar.gz', 'capture_dir']
        self.bodybag.collect_dir("bag_dir", "/capture_dir_parent/capture_dir")
        mock_call.assert_called_with(mock_args)

    @mock.patch('coroner.open', create=True)
    @mock.patch('coroner.FDSCoroner.prep_data_dir')
    @mock.patch('coroner.logging')
    @mock.patch('coroner.subprocess.Popen')
    def test_collect_cmd_success(self,
                                 mock_subproc_popen,
                                 mock_log,
                                 mock_prep_data_dir,
                                 mock_open):
        process_mock = mock.Mock()
        attrs = {
                'communicate.return_value': ('output', 'error'),
                'returncode': 0
        }
        process_mock.configure_mock(**attrs)
        mock_subproc_popen.return_value = process_mock
        mock_prep_data_dir.return_value = True
        mock_open.return_value = mock.MagicMock(spec=file)
        self.bodybag.collect_cmd('workingcmd')
        mock_args = ['/usr/bin/nice', '-n', '19', 'workingcmd']
        mock_subproc_popen.assert_called_once_with(
                mock_args,
                stderr=-1,
                stdout=-1
        )
        self.assertTrue(mock_log.info.called)

    @mock.patch('coroner.open', create=True)
    @mock.patch('coroner.FDSCoroner.prep_data_dir')
    @mock.patch('coroner.logging')
    @mock.patch('coroner.subprocess.Popen')
    def test_collect_cmd_failure(self,
                                 mock_subproc_popen,
                                 mock_log,
                                 mock_prep_data_dir,
                                 mock_open):
        process_mock = mock.Mock()
        attrs = {
                'communicate.return_value': ('output', 'error'),
                'returncode': 1
        }
        process_mock.configure_mock(**attrs)
        mock_subproc_popen.return_value = process_mock
        mock_prep_data_dir.return_value = True
        mock_open.return_value = mock.MagicMock(spec=file)
        self.bodybag.collect_cmd('workingcmd')
        mock_args = ['/usr/bin/nice', '-n', '19', 'workingcmd']
        mock_subproc_popen.assert_called_once_with(
                mock_args,
                stderr=-1,
                stdout=-1
        )
        self.assertTrue(mock_log.error.called)

    @mock.patch('coroner.glob')
    @mock.patch('coroner.FDSCoroner.get_dir_path')
    @mock.patch('coroner.subprocess.call')
    def test_compress_sparse_files(self,
                                   mock_subproc_call,
                                   mock_get_dir_path,
                                   mock_glob):
        process_mock = mock.Mock()
        attrs = { 'returncode': 0 }
        process_mock.configure_mock(**attrs)
        mock_subproc_call.return_value = process_mock
        mock_get_dir_path.return_value = '/bagdir/somedir'
        mock_glob.glob.return_value = [ '/bagdir/somedir/somefile' ]
        self.bodybag.compress_sparse_files('somedir')
        mock_args = ['/bin/tar', '--remove-files', '-C', '/bagdir/somedir',
                '-Sczf', '/bagdir/somedir/somefile.tar.gz', 'somefile' ]
        mock_subproc_call.assert_called_once_with( mock_args )

    @mock.patch('coroner.FDSCoroner.compress_sparse_files')
    @mock.patch('coroner.FDSCoroner.collect_paths')
    def test_collect_cores(self, mock_collect_paths, mock_compress):
        paths = ['/onepath', '/two/path']
        self.bodybag.collect_cores(paths)
        mock_collect_paths.assert_called_once_with('cores', paths)
        mock_compress.assert_called_once_with('cores')

    @mock.patch('coroner.FDSCoroner.collect_dir')
    def test_collect_cli_dirs(self, mock_collect_dir):
        dirlist = [ 'name:directory', 'name2:directory2' ]
        self.bodybag.collect_cli_dirs(dirlist)
        mock_collect_dir.assert_called_twice_with(
                directory='name',
                path='directory'
        )

    @mock.patch('coroner.FDSCoroner.collect_cmd')
    def test_collect_cli_dirs(self, mock_collect_cmd):
        cmdlist = [ 'command1', 'command_2' ]
        self.bodybag.collect_cli_cmds(cmdlist)
        mock_collect_cmd.assert_called_twice_with(command='command1')

    @mock.patch('coroner.subprocess.call')
    @mock.patch('coroner.logging')
    def test_close(self, mock_logging, mock_subproc_call):
        process_mock = mock.Mock()
        attrs = { 'returncode': 0 }
        process_mock.configure_mock(**attrs)
        mock_subproc_call.return_value = process_mock
        self.bodybag.destdir = '/tmp'
        self.bodybag.bodybag = '/tmp/blah'
        self.bodybag.close()
        mock_args = ['/bin/tar', '--remove-files', '-C', '/tmp',
                '-czf', '/tmp/blah.tar.gz', 'blah' ]
        mock_subproc_call.assert_called_once_with(mock_args)

if __name__ == '__main__':
    unittest.main()
