import logging
import unittest

import ec2
import domain

class TestDomain(unittest.TestCase):
    
    logging.basicConfig(level=logging.INFO,
                        format='%(asctime)s %(name)-12s %(levelname)-8s %(message)s',
                        datefmt='%m-%d %H:%M')
    log = logging.getLogger(__name__)
        
    def setUp(self):
        pass
        
    def test_constructor(self):
        # create a Domain with an invalid cluster size
        dobj = domain.Domain(name="test_domain", cluster_size=1)
        
        self.assertEqual(dobj.cluster_size, 1)
        self.assertEqual(len(dobj.nodes), 1)
        self.assertEqual(len(dobj.om_nodes), 1)
        self.assertEqual(dobj.nodes[0], dobj.om_nodes[0])
        self.assertEqual(dobj.om_nodes[0].get_instance_status(), 'running')
        
        dobj.terminate()
    
    def test_create_ec2_multiple_increment(self):
        dobj = domain.Domain(name="test_domain", cluster_size=4)
        # assert cluster size is 4
        self.assertEqual(dobj.cluster_size, 4)
        self.assertEqual(dobj.has_started, True)
        # assert all the 4 nodes are running
        for node in dobj.nodes:
            self.assertEqual(node.get_instance_status(), 'running')
        
        # increment the number of nodes to 16
        dobj.create_ec2_multiple_increment(12)
        self.assertEqual(dobj.cluster_size, 16)
        # assert all the 16 nodes are running
        for node in dobj.nodes:
            self.assertEqual(node.get_instance_status(), 'running')
        
        dobj.terminate()
        self.assertEqual(dobj.cluster_size, 0)
        self.assertEqual(dobj.has_started, False)
        
    def test_create_ec2_single_increment(self):
        dobj = domain.Domain(name="test_domain")
        # assert cluster size is 4
        self.assertEqual(dobj.cluster_size, 4)
        self.assertEqual(dobj.has_started, True)
        # assert all the 4 nodes are running
        for node in dobj.nodes:
            self.assertEqual(node.get_instance_status(), 'running')
        
        # increment the number of nodes to 16
        dobj.create_ec2_single_increment(12)
        self.assertEqual(dobj.cluster_size, 16)
        # assert all the 16 nodes are running
        for node in dobj.nodes:
            self.assertEqual(node.get_instance_status(), 'running')
        
        dobj.terminate()
        self.assertEqual(dobj.cluster_size, 0)
        self.assertEqual(dobj.has_started, False)
        
    def test_halt(self):
        dobj = domain.Domain(name="test_domain")
        # assert cluster size is 4
        self.assertEqual(dobj.cluster_size, 4)
        self.assertEqual(dobj.has_started, True)
        # assert all the 4 nodes are running
        for node in dobj.nodes:
            self.assertEqual(node.get_instance_status(), 'running')

        dobj.halt()
        for node in dobj.nodes:
            self.assertEqual(node.get_instance_status(), 'stopped')
            
        dobj.resume()
        for node in dobj.nodes:
            self.assertEqual(node.get_instance_status(), 'running')
            
        dobj.terminate()

if __name__ == '__main__':
    unittest.main()