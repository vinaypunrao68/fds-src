import logging
import unittest
import pdb
import sys
import fdslib.FDSServiceUtils as FDSServiceUtils
import TestCase
from fdslib.TestUtils import findNodeFromInv


#AM classes to start/stop/add/remove/kill AM Test
class TestStartAM(TestCase.FDSTestCase):
    def __init__(self,parameters=None, node=None):
        super(self.__class__, self).__init__(parameters,
                                            self.__class__.__name__,
                                            self.test_start,
                                            "Start AM test")

        self.passedNode = node
        #get om IP address from cfg to pass to each test
        fdscfg = self.parameters["fdscfg"]
        om_node = fdscfg.rt_om_node
        self.omip = om_node.nd_conf_dict['ip']

        ##get node IP address from cfg to pass to each test
        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            if self.passedNode is not None:
                n = findNodeFromInv(nodes, self.passedNode)

        self.nodeip = n.nd_conf_dict['ip']
        self.am_obj = FDSServiceUtils.AMService(self.omip, self.nodeip)

    def test_start(self):
        #get om IP address from cfg to pass to each test
        if self.am_obj.start('{}'.format(self.nodeip)) == True:
            return True
        else:
            return False

class TestStopAM(TestCase.FDSTestCase):
    def __init__(self,parameters=None, node=None):
        super(self.__class__, self).__init__(parameters,
                                            self.__class__.__name__,
                                            self.test_stop,
                                            "Stop AM test")
        #get om IP address from cfg to pass to each test
        fdscfg = self.parameters["fdscfg"]
        om_node = fdscfg.rt_om_node
        self.omip = om_node.nd_conf_dict['ip']

        #get node IP address from cfg to pass to each test
        self.passedNode = node
        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            if self.passedNode is not None:
                n = findNodeFromInv(nodes, self.passedNode)

        self.nodeip = n.nd_conf_dict['ip']
        self.am_obj = FDSServiceUtils.AMService(self.omip, self.nodeip)


    def test_stop(self):
        if self.am_obj.stop('{}'.format(self.nodeip)) == True:
            return True
        else:
            return False

class TestKillAM(TestCase.FDSTestCase):
    def __init__(self,parameters=None, node=None):
        super(self.__class__, self).__init__(parameters,
                                            self.__class__.__name__,
                                            self.test_kill,
                                            "Kill AM test")
        #get om IP address from cfg to pass to each test
        fdscfg = self.parameters["fdscfg"]
        om_node = fdscfg.rt_om_node
        self.omip = om_node.nd_conf_dict['ip']

        #get node IP address from cfg to pass to each test
        self.passedNode = node
        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            if self.passedNode is not None:
                n = findNodeFromInv(nodes, self.passedNode)

        self.nodeip = n.nd_conf_dict['ip']
        self.am_obj = FDSServiceUtils.AMService(self.omip, self.nodeip)


    def test_kill(self):
        if self.am_obj.kill('{}'.format(self.nodeip)) == True:
            return True
        else:
            return False

class TestAddAM(TestCase.FDSTestCase):
    def __init__(self,parameters=None, node=None):
        super(self.__class__, self).__init__(parameters,
                                            self.__class__.__name__,
                                            self.test_add,
                                            "Add AM test")
        #get om IP address from cfg to pass to each test
        fdscfg = self.parameters["fdscfg"]
        om_node = fdscfg.rt_om_node
        self.omip = om_node.nd_conf_dict['ip']

        #get node IP address from cfg to pass to each test
        self.passedNode = node
        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            if self.passedNode is not None:
                n = findNodeFromInv(nodes, self.passedNode)

        self.nodeip = n.nd_conf_dict['ip']
        self.am_obj = FDSServiceUtils.AMService(self.omip, self.nodeip)


    def test_add(self):
        if self.am_obj.add('{}'.format(self.nodeip)) == True:
            return True
        else:
            return False

class TestRemoveAM(TestCase.FDSTestCase):
    def __init__(self,parameters=None, node=None):
        super(self.__class__, self).__init__(parameters,
                                            self.__class__.__name__,
                                            self.test_remove,
                                            "Remove AM test")
        #get om IP address from cfg to pass to each test
        fdscfg = self.parameters["fdscfg"]
        om_node = fdscfg.rt_om_node
        self.omip = om_node.nd_conf_dict['ip']

        #get node IP address from cfg to pass to each test
        self.passedNode = node
        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            if self.passedNode is not None:
                n = findNodeFromInv(nodes, self.passedNode)

        self.nodeip = n.nd_conf_dict['ip']
        self.am_obj = FDSServiceUtils.AMService(self.omip, self.nodeip)


    def test_remove(self):
        if self.am_obj.remove('{}'.format(self.nodeip)) == True:
            return True
        else:
            return False

#DM classes to start/stop/add/remove/kill DM Tests
class TestStartDM(TestCase.FDSTestCase):
    def __init__(self,parameters=None, node=None):
        super(self.__class__, self).__init__(parameters,
                                            self.__class__.__name__,
                                            self.test_start,
                                            "Start DM test")
        #get om IP address from cfg to pass to each test
        fdscfg = self.parameters["fdscfg"]
        om_node = fdscfg.rt_om_node
        self.omip = om_node.nd_conf_dict['ip']

        #get node IP address from cfg to pass to each test
        self.passedNode = node
        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            if self.passedNode is not None:
                n = findNodeFromInv(nodes, self.passedNode)

        self.nodeip = n.nd_conf_dict['ip']
        self.dm_obj = FDSServiceUtils.DMService(self.omip, self.nodeip)


    def test_start(self):
        if self.dm_obj.start('{}'.format(self.nodeip)) == True:
            return True
        else:
            return False

class TestStopDM(TestCase.FDSTestCase):
    def __init__(self,parameters=None):
        super(self.__class__, self).__init__(parameters,
                                            self.__class__.__name__,
                                            self.test_stop,
                                            "Stop DM test")

        #get om IP address from cfg to pass to each test
        fdscfg = self.parameters["fdscfg"]
        om_node = fdscfg.rt_om_node
        self.omip = om_node.nd_conf_dict['ip']

        #get node IP address from cfg to pass to each test
        self.passedNode = node
        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            if self.passedNode is not None:
                n = findNodeFromInv(nodes, self.passedNode)

        self.nodeip = n.nd_conf_dict['ip']
        self.dm_obj = FDSServiceUtils.DMService(self.omip, self.nodeip)


    def test_stop(self):
        if self.dm_obj.stop('{}'.format(self.nodeip)) == True:
            return True
        else:
            return False

class TestKillDM(TestCase.FDSTestCase):
    def __init__(self,parameters=None):
        super(self.__class__, self).__init__(parameters,
                                            self.__class__.__name__,
                                            self.test_kill,
                                            "Kill DM test")

        #get om IP address from cfg to pass to each test
        fdscfg = self.parameters["fdscfg"]
        om_node = fdscfg.rt_om_node
        self.omip = om_node.nd_conf_dict['ip']

        #get node IP address from cfg to pass to each test
        self.passedNode = node
        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            if self.passedNode is not None:
                n = findNodeFromInv(nodes, self.passedNode)

        self.nodeip = n.nd_conf_dict['ip']
        self.dm_obj = FDSServiceUtils.DMService(self.omip, self.nodeip)


    def test_kill(self):
        if self.dm_obj.kill('{}'.format(self.nodeip)) == True:
            return True
        else:
            return False

class TestAddDM(TestCase.FDSTestCase):
    def __init__(self,parameters=None):
        super(self.__class__, self).__init__(parameters,
                                            self.__class__.__name__,
                                            self.test_add,
                                            "Add DM test")

        #get om IP address from cfg to pass to each test
        fdscfg = self.parameters["fdscfg"]
        om_node = fdscfg.rt_om_node
        self.omip = om_node.nd_conf_dict['ip']

        #get node IP address from cfg to pass to each test
        self.passedNode = node
        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            if self.passedNode is not None:
                n = findNodeFromInv(nodes, self.passedNode)

        self.nodeip = n.nd_conf_dict['ip']
        self.dm_obj = FDSServiceUtils.DMService(self.omip, self.nodeip)


    def test_add(self):
        if self.dm_obj.add('{}'.format(self.nodeip)) == True:
            return True
        else:
            return False

class TestRemoveDM(TestCase.FDSTestCase):
    def __init__(self,parameters=None):
        super(self.__class__, self).__init__(parameters,
                                            self.__class__.__name__,
                                            self.test_remove,
                                            "Remove DM test")

        #get om IP address from cfg to pass to each test
        fdscfg = self.parameters["fdscfg"]
        om_node = fdscfg.rt_om_node
        self.omip = om_node.nd_conf_dict['ip']

        #get node IP address from cfg to pass to each test
        self.passedNode = node
        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            if self.passedNode is not None:
                n = findNodeFromInv(nodes, self.passedNode)

        self.nodeip = n.nd_conf_dict['ip']
        self.dm_obj = FDSServiceUtils.DMService(self.omip, self.nodeip)


    def test_remove(self):
        if self.dm_obj.remove('{}'.format(self.nodeip)) == True:
            return True
        else:
            return False

#SM classes to start/stop/add/remove/kill SM Tests
class TestStartSM(TestCase.FDSTestCase):
    def __init__(self,parameters=None, node=None):
        super(self.__class__, self).__init__(parameters,
                                            self.__class__.__name__,
                                            self.test_start,
                                            "Start SM test")

        #get om IP address from cfg to pass to each test
        fdscfg = self.parameters["fdscfg"]
        om_node = fdscfg.rt_om_node
        self.omip = om_node.nd_conf_dict['ip']

        #get node IP address from cfg to pass to each test
        self.passedNode = node
        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            if self.passedNode is not None:
                n = findNodeFromInv(nodes, self.passedNode)

        self.nodeip = n.nd_conf_dict['ip']
        self.sm_obj = FDSServiceUtils.SMService(self.omip, self.nodeip)

    def test_start(self):
        if self.sm_obj.start('{}'.format(self.nodeip)) == True:
            return True
        else:
            return False

class TestStopSM(TestCase.FDSTestCase):
    def __init__(self,parameters=None):
        super(self.__class__, self).__init__(parameters,
                                            self.__class__.__name__,
                                            self.test_stop,
                                            "Stop SM test")

        #get om IP address from cfg to pass to each test
        fdscfg = self.parameters["fdscfg"]
        om_node = fdscfg.rt_om_node
        self.omip = om_node.nd_conf_dict['ip']

        #get node IP address from cfg to pass to each test
        self.passedNode = node
        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            if self.passedNode is not None:
                n = findNodeFromInv(nodes, self.passedNode)

        self.nodeip = n.nd_conf_dict['ip']
        self.sm_obj = FDSServiceUtils.SMService(self.omip, self.nodeip)

    def test_stop(self):
        if self.sm_obj.stop('{}'.format(self.nodeip)) == True:
            return True
        else:
            return False

class TestKillSM(TestCase.FDSTestCase):
    def __init__(self,parameters=None):
        super(self.__class__, self).__init__(parameters,
                                            self.__class__.__name__,
                                            self.test_kill,
                                            "Kill SM test")

        #get om IP address from cfg to pass to each test
        fdscfg = self.parameters["fdscfg"]
        om_node = fdscfg.rt_om_node
        self.omip = om_node.nd_conf_dict['ip']

        #get node IP address from cfg to pass to each test
        self.passedNode = node
        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            if self.passedNode is not None:
                n = findNodeFromInv(nodes, self.passedNode)

        self.nodeip = n.nd_conf_dict['ip']
        self.sm_obj = FDSServiceUtils.SMService(self.omip, self.nodeip)


    def test_kill(self):
        if self.sm_obj.kill('{}'.format(self.nodeip)) == True:
            return True
        else:
            return False

class TestAddSM(TestCase.FDSTestCase):
    def __init__(self,parameters=None):
        super(self.__class__, self).__init__(parameters,
                                            self.__class__.__name__,
                                            self.test_add,
                                            "Add SM test")

        #get om IP address from cfg to pass to each test
        fdscfg = self.parameters["fdscfg"]
        om_node = fdscfg.rt_om_node
        self.omip = om_node.nd_conf_dict['ip']

        #get node IP address from cfg to pass to each test
        self.passedNode = node
        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            if self.passedNode is not None:
                n = findNodeFromInv(nodes, self.passedNode)

        self.nodeip = n.nd_conf_dict['ip']
        self.sm_obj = FDSServiceUtils.SMService(self.omip, self.nodeip)


    def test_add(self):
        if self.sm_obj.add('{}'.format(self.nodeip)) == True:
            return True
        else:
            return False

class TestRemoveSM(TestCase.FDSTestCase):
    def __init__(self,parameters=None):
        super(self.__class__, self).__init__(parameters,
                                            self.__class__.__name__,
                                            self.test_remove,
                                            "Remove SM test")

        #get om IP address from cfg to pass to each test
        fdscfg = self.parameters["fdscfg"]
        om_node = fdscfg.rt_om_node
        self.omip = om_node.nd_conf_dict['ip']

        #get node IP address from cfg to pass to each test
        self.passedNode = node
        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            if self.passedNode is not None:
                n = findNodeFromInv(nodes, self.passedNode)

        self.nodeip = n.nd_conf_dict['ip']
        self.sm_obj = FDSServiceUtils.SMService(self.omip, self.nodeip)


    def test_remove(self):
        if self.sm_obj.remove('{}'.format(self.nodeip)) == True:
            return True
        else:
            return False

#PM classes to start/stop/add/remove/kill PM Tests
class TestStartPM(TestCase.FDSTestCase):
    def __init__(self,parameters=None, node=None):
        super(self.__class__, self).__init__(parameters,
                                            self.__class__.__name__,
                                            self.test_start,
                                            "Start PM test")

        #get om IP address from cfg to pass to each test
        fdscfg = self.parameters["fdscfg"]
        om_node = fdscfg.rt_om_node
        self.omip = om_node.nd_conf_dict['ip']

        #get node IP address from cfg to pass to each test
        self.passedNode = node
        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            if self.passedNode is not None:
                n = findNodeFromInv(nodes, self.passedNode)

        self.nodeip = n.nd_conf_dict['ip']
        self.pm_obj = FDSServiceUtils.PMService(self.omip, self.nodeip)


    def test_start(self):
        if self.pm_obj.start('{}'.format(self.nodeip)) == True:
            return True
        else:
            return False

class TestStopPM(TestCase.FDSTestCase):
    def __init__(self,parameters=None):
        super(self.__class__, self).__init__(parameters,
                                            self.__class__.__name__,
                                            self.test_stop,
                                            "Stop PM test")

        #get om IP address from cfg to pass to each test
        fdscfg = self.parameters["fdscfg"]
        om_node = fdscfg.rt_om_node
        self.omip = om_node.nd_conf_dict['ip']

        #get node IP address from cfg to pass to each test
        self.passedNode = node
        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            if self.passedNode is not None:
                n = findNodeFromInv(nodes, self.passedNode)

        self.nodeip = n.nd_conf_dict['ip']
        self.pm_obj = FDSServiceUtils.PMService(self.omip, self.nodeip)


    def test_stop(self):
        if self.pm_obj.stop('{}'.format(self.nodeip)) == True:
            return True
        else:
            return False

class TestKillPM(TestCase.FDSTestCase):
    def __init__(self,parameters=None):
        super(self.__class__, self).__init__(parameters,
                                            self.__class__.__name__,
                                            self.test_kill,
                                            "Kill PM test")

        #get om IP address from cfg to pass to each test
        fdscfg = self.parameters["fdscfg"]
        om_node = fdscfg.rt_om_node
        self.omip = om_node.nd_conf_dict['ip']

        #get node IP address from cfg to pass to each test
        self.passedNode = node
        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            if self.passedNode is not None:
                n = findNodeFromInv(nodes, self.passedNode)

        self.nodeip = n.nd_conf_dict['ip']
        self.pm_obj = FDSServiceUtils.PMService(self.omip, self.nodeip)


    def test_kill(self):
        if self.pm_obj.kill('{}'.format(self.nodeip)) == True:
            return True
        else:
            return False

class TestAddPM(TestCase.FDSTestCase):
    def __init__(self,parameters=None):
        super(self.__class__, self).__init__(parameters,
                                            self.__class__.__name__,
                                            self.test_add,
                                            "Add PM test")

        #get om IP address from cfg to pass to each test
        fdscfg = self.parameters["fdscfg"]
        om_node = fdscfg.rt_om_node
        self.omip = om_node.nd_conf_dict['ip']

        #get node IP address from cfg to pass to each test
        self.passedNode = node
        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            if self.passedNode is not None:
                n = findNodeFromInv(nodes, self.passedNode)

        self.nodeip = n.nd_conf_dict['ip']
        self.pm_obj = FDSServiceUtils.PMService(self.omip, self.nodeip)


    def test_add(self):
        if self.pm_obj.add('{}'.format(self.nodeip)) == True:
            return True
        else:
            return False

class TestRemovePM(TestCase.FDSTestCase):
    def __init__(self,parameters=None):
        super(self.__class__, self).__init__(parameters,
                                            self.__class__.__name__,
                                            self.test_remove,
                                            "Remove PM test")

        #get om IP address from cfg to pass to each test
        fdscfg = self.parameters["fdscfg"]
        om_node = fdscfg.rt_om_node
        self.omip = om_node.nd_conf_dict['ip']

        #get node IP address from cfg to pass to each test
        self.passedNode = node
        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            if self.passedNode is not None:
                n = findNodeFromInv(nodes, self.passedNode)

        self.nodeip = n.nd_conf_dict['ip']
        self.pm_obj = FDSServiceUtils.PMService(self.omip, self.nodeip)


    def test_remove(self):
        if self.pm_obj.remove('{}'.format(self.nodeip)) == True:
            return True
        else:
            return False

if __name__ == '__main__':
    TestCase.FDSTestCase.fdsGetCmdLineConfigs(sys.argv)
    unittest.main()

