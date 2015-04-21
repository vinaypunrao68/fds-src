import logging
import unittest

import ec2

class TestEC2(unittest.TestCase):
    
    logging.basicConfig(level=logging.INFO,
                        format='%(asctime)s %(name)-12s %(levelname)-8s %(message)s',
                        datefmt='%m-%d %H:%M')
    log = logging.getLogger(__name__)
        
    def setUp(self):
        self.log.info(TestEC2.setUp.__name__)
        self.ec2obj = ec2.EC2(name="testlib_unittest")
        self.assertEqual(self.ec2obj.instance, None)
        self.assertEqual(self.ec2obj.reservation, None)
        self.assertEqual(self.ec2obj.volumes, {})
        self.assertEqual(self.ec2obj.get_instance_status(), None)
        
    def test_start_instance(self):
        self.log.info(TestEC2.test_start_instance.__name__)
        if self.ec2obj.instance == None:
            self.ec2obj.start_instance()
        self.assertNotEqual(self.ec2obj.instance, None)
        self.assertEquals(self.ec2obj.instance.state, 'running')
        self.assertNotEqual(self.ec2obj.get_instance_status(), None)
        self.ec2obj.terminate_instance()
        
    def test_stop_instance(self):
        self.log.info(TestEC2.test_stop_instance.__name__)
        if self.ec2obj.instance == None:
            self.ec2obj.start_instance()
        self.assertNotEqual(self.ec2obj.instance, None)
        self.ec2obj.stop_instance()
        self.assertEqual(self.ec2obj.get_instance_status(), 'stopped')
        self.ec2obj.terminate_instance()
    
    def test_terminate_instance(self):
        self.log.info(TestEC2.test_terminate_instance.__name__)
        if self.ec2obj.instance == None:
            self.ec2obj.start_instance()
        self.ec2obj.terminate_instance()
        self.assertEqual(self.ec2obj.get_instance_status(), 'terminated')
        
    def test_attach_volume(self):
        self.log.info(TestEC2.test_attach_volume.__name__)
        self.ec2obj.start_instance()
        vol_id = self.ec2obj.attach_volume(100, "/dev/sdx")
        self.assertEqual(self.ec2obj.get_volumes_status(vol_id), 'attached')
        self.ec2obj.terminate_instance()

if __name__ == '__main__':
    unittest.main()