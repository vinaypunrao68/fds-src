import logging
import unittest
import time

import ec2

class TestEC2(unittest.TestCase):
    
    logging.basicConfig(level=logging.INFO,
                        format='%(asctime)s %(name)-12s %(levelname)-8s %(message)s',
                        datefmt='%m-%d %H:%M')
    log = logging.getLogger(__name__)
        
    def test_default(self):
        self.log.info(TestEC2.setUp.__name__)
        self.ec2obj = ec2.EC2(name="testlib_unittest")
        self.assertEqual(self.ec2obj.instance, None)
        self.assertEqual(self.ec2obj.reservation, None)
        self.assertEqual(self.ec2obj.volumes, {})
        self.assertEqual(self.ec2obj.get_instance_status(), None)
        
    def test_start_instance(self):
        self.log.info(TestEC2.test_start_instance.__name__)
        ec2obj = ec2.EC2(name="testlib_unittest")
        ec2obj.start_instance()
        self.assertNotEqual(ec2obj.instance, None)
        self.assertEquals(ec2obj.instance.state, 'running')
        self.assertNotEqual(ec2obj.get_instance_status(), None)
        ec2obj.terminate_instance()
        
    def test_stop_instance(self):
        self.log.info(TestEC2.test_stop_instance.__name__)
        ec2obj = ec2.EC2(name="testlib_unittest")
        ec2obj.start_instance()
        self.assertNotEqual(ec2obj.instance, None)
        ec2obj.stop_instance()
        self.assertEqual(ec2obj.get_instance_status(), 'stopped')
        ec2obj.terminate_instance()
    
    def test_resume_instance(self):
        self.log.info(TestEC2.test_stop_instance.__name__)
        ec2obj = ec2.EC2(name="testlib_unittest")
        ec2obj.start_instance()
        self.assertNotEqual(ec2obj.instance, None)
        ec2obj.stop_instance()
        self.assertEqual(ec2obj.get_instance_status(), 'stopped')
        # resume the instance
        ec2obj.resume_instance()
        self.assertEquals(ec2obj.instance.state, 'running')
        ec2obj.terminate_instance()
        
    def test_terminate_instance(self):
        self.log.info(TestEC2.test_terminate_instance.__name__)
        ec2obj = ec2.EC2(name="testlib_unittest")
        ec2obj.start_instance()
        ec2obj.terminate_instance()
        self.assertEqual(ec2obj.get_instance_status(), 'terminated')
        
    def test_attach_volume(self):
        ec2obj = ec2.EC2(name="testlib_unittest")
        self.log.info(TestEC2.test_attach_volume.__name__)
        ec2obj.start_instance()
        status = ec2obj.attach_volume(10, "/dev/sdx")
        self.assertTrue(status)
        ec2obj.terminate_instance()

    def test_detach_volume(self):
        ec2obj = ec2.EC2(name="testlib_unittest")
        self.log.info(TestEC2.test_detach_volume.__name__)
        ec2obj.start_instance()
        status = ec2obj.attach_volume(10, "/dev/sdx")
        self.assertTrue(status)
        status = ec2obj.detach_volume("/dev/sdx")
        ec2obj.terminate_instance()
        for device, volume in ec2obj.volumes.iteritems():
            ec2obj.delete_volume(volume)
        
    def test_delete_volume(self):
        ec2obj = ec2.EC2(name="testlib_unittest")
        self.log.info(TestEC2.test_delete_volume.__name__)
        ec2obj.start_instance()
        vol = ec2obj.ec2_conn.create_volume(10, ec2obj.instance.placement)
        while vol.status != 'available':
                time.sleep(20)
                vol.update()
        ec2obj.delete_volume(vol)
        ec2obj.terminate_instance()

if __name__ == '__main__':
    unittest.main()