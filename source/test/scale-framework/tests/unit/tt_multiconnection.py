import mock
import unittest

import testsets.testcase as testcase
import testsets.testcases.test_multiconnections as multiconn

class TTMultiConnections(unittest.TestCase):
    
    def setUp(self):
        self.multiconn = multiconn.TestMultiConnections(None, None, '10.2.10.200')
        
    def test_multiconnections(self):
        print self.multiconn
        
if __name__ == '__main__':
    unittest.main()