import logging
import unittest

import ec2

class TestDomain(unittest.TestCase):
    
    logging.basicConfig(level=logging.INFO,
                        format='%(asctime)s %(name)-12s %(levelname)-8s %(message)s',
                        datefmt='%m-%d %H:%M')
    log = logging.getLogger(__name__)
        
    def setUp(self):
        pass

if __name__ == '__main__':
    unittest.main()