##############################################################################
# author: Philippe Ribeiro                                                   #
# email: philippe@formationds                                                #
# file: test_ec2.py                                                          #
##############################################################################

import logging
import unittest

import ec2

class TestEC2(unittest.TestCase):
    
    def setUp(self):
        self.instance = ec2.EC2(name="test_ec2")
        
    def test_check_status(self):
        self.assertEqual(self.instance.status, "running")
        
        
if __name__ == "__main__":
    unittest.main()