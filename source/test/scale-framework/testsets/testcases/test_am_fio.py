#########################################################################
# @Copyright: 2015 by Formation Data Systems, Inc.                      #
# @file: test_am_fio.py                                                 #
# @author: Philippe Ribeiro                                             #
# @email: philippe@formationds.com                                      #
#########################################################################
import config
import ssh
import subprocess
import sys

import testsets.testcase as testcase

class TestAMFIO(testcase.FDSTestCase):
    
    def __init__(self, parameters=None, config_file=None,
                om_ip_address=None):
        super(TestAMFIO, self).__init__(parameters=parameters,
                                        config_file=config_file,
                                        om_ip_address=om_ip_address)
        self.my_ssh = ssh.SSHConn(om_ip_address, "root", "passwd")
        
    def runTest(self):
        # execute the initial test
        try:
            # please use fdscli to create volumes
            create_volume = "cd %s && ./fdsconsole.py volume create  volume1 --vol-type block --blk-dev-size 10485760" % (config.FDS_SBIN)
            stdin, stdout, stderr = self.my_ssh.ssh_conn.exec_command(create_volume)
            attach_volume = "python %s attach 10.2.10.200 volume1"  % config.NDBADM
            subprocess.call([attach_volume], shell=True)
            modify_volume = "cd %s && ./fdsconsole.py volume modify volume1 --minimum 0 --maximum 10000 --priority 1" % (config.FDS_SBIN)
            stdin, stdout, stderr = self.my_ssh.ssh_conn.exec_command(modify_volume)
            self.test_datasets()
            self.my_ssh.ssh_conn.close()
        except Exception, e:
            self.log.exception(e)
            self.test_passed = False
        finally:
            self.reportTestCaseResult(self.test_passed)
        
    def test_datasets(self):
        try:
            self.log.info("hello")
            datasets = ["10","128", "16"]
            cmds = [
               'fio --name=test  --ioengine=libaio --iodepth=16 --direct=1 --size=%sm  --rw=write --filename=/dev/nbd15 --numjobs=1',
               'fio --name=test  --ioengine=libaio --iodepth=16 --direct=1 --size=%sm  --rw=randread --filename=/dev/nbd15 --numjobs=1 --runtime=60'
            ]
            for cmd in cmds:
                for dataset in datasets:
                    current = cmd % (dataset)
                    self.log.info(current)
                    subprocess.call([current], shell=True)
            self.test_passed = True
        except Exception:
            self.test_passed = False
