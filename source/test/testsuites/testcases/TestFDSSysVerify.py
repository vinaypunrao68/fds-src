#!/usr/bin/python
#
# Copyright 2014 by Formation Data Systems, Inc.
#

# FDS test-case pattern requirements.
import unittest
import traceback
import TestCase

# Module-specific requirements
import sys
import os
import logging
import filecmp
import os.path

def fileSearch(searchFile, searchString, occurrences):
    """
    Search the given file for the given search string
    verifying whether it occurs the specified number
    of times.
    """
    log = logging.getLogger('TestFDSSysVerify' + '.' + 'fileSearch')

    # Verify the file exists.
    if not os.path.isfile(searchFile):
        log.error("File %s is not found." % (searchFile))
        return False

    f = open(searchFile, "r")
    searchLines = f.readlines()
    f.close()

    occur = 0
    log.info("Searching for exactly %d occurrence(s) of string [%s]." % (occurrences, searchString))
    for line in searchLines:
        if searchString in line:
            occur = occur + 1
            log.info("Found occurrence %d: %s." % (occur, line))

    if occur == occurrences:
        return True
    else:
        return False


def are_dir_trees_equal(dir1, dir2, logDiff=False):
    """
    Compare two directories recursively. Files in each directory are
    assumed to be equal if their names and contents are equal.

    @param dir1: First directory path
    @param dir2: Second directory path
    @param logDiff: If True, log the differences found.

    @return: True if the directory trees are the same and
        there were no errors while accessing the directories or files,
        False otherwise.
    """
    log = logging.getLogger('TestFDSSysVerify' + '.' + 'are_dir_trees_equal')

    if not os.path.exists(dir1):
        if logDiff:
            log.error("Directory %s does not exist (or maybe no permissions to read it)." % (dir1))
        return False

    if not os.path.exists(dir2):
        if logDiff:
            log.error("Directory %s does not exist (or maybe no permissions to read it)." % (dir2))
        return False

    dirs_cmp = filecmp.dircmp(dir1, dir2)

    if len(dirs_cmp.left_only) > 0:
        if logDiff:
            log.error("Files/Directories in %s do not appear in %s." % (dir1, dir2))
            log.error(dirs_cmp.left_only)
        return False

    if len(dirs_cmp.right_only) > 0:
        if logDiff:
            log.error("Files/Directories in %s do not appear in %s." % (dir2, dir1))
            log.error(dirs_cmp.right_only)
        return False

    if len(dirs_cmp.funny_files) > 0:
        if logDiff:
            log.error("Files in both %s and %s could not be compared." % (dir2, dir1))
            log.error(dirs_cmp.funny_files)
        return False

    (_, mismatch, errors) = filecmp.cmpfiles(
        dir1, dir2, dirs_cmp.common_files, shallow=False)
    if len(mismatch) > 0:
        if logDiff:
            log.error("Mismatched files between %s and %s." % (dir2, dir1))
            log.error(mismatch)
        return False

    if len(errors) > 0:
        if logDiff:
            log.error("Unable to compare files between %s and %s." % (dir2, dir1))
            log.error(errors)
        return False

    for common_dir in dirs_cmp.common_dirs:
        new_dir1 = os.path.join(dir1, common_dir)
        new_dir2 = os.path.join(dir2, common_dir)
        if not are_dir_trees_equal(new_dir1, new_dir2, logDiff):
            return False

    return True


# This class contains the attributes and methods to test
# whether there exists the expected initial DMT migration
# completion OM log entries for a 1-node cluster.
class TestInitial1NodeDMTMigration(TestCase.FDSTestCase):
    def __init__(self, parameters=None):
        super(TestInitial1NodeDMTMigration, self).__init__(parameters)


    def runTest(self):
        test_passed = True

        if TestCase.pyUnitTCFailure:
            self.log.warning("Skipping Case %s. stop-on-fail/failfast set and a previous test case has failed." %
                             self.__class__.__name__)
            return unittest.skip("stop-on-fail/failfast set and a previous test case has failed.")
        else:
            self.log.info("Running Case %s." % self.__class__.__name__)

        try:
            if not self.test_Initial1NodeDMTMigration():
                test_passed = False
        except Exception as inst:
            self.log.error("Initial 1-node DMT migration caused exception:")
            self.log.error(traceback.format_exc())
            test_passed = False

        super(self.__class__, self).reportTestCaseResult(test_passed)

        # If there is any test fixture teardown to be done, do it here.

        if self.parameters["pyUnit"]:
            self.assertTrue(test_passed)
        else:
            return test_passed


    def test_Initial1NodeDMTMigration(self):
        """
        Test Case:
        Attempt locate in OM's log logged statements indicating
        successful completion of the initial DMT migration for
        a 1-node cluster.
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]
        om_node = fdscfg.rt_om_node
        fds_dir = om_node.nd_conf_dict['fds_root']
        log_file = fds_dir + "/var/logs/om.log_0.log"

        if fileSearch(log_file, "OM_PmAgent: will send node activate message to Node-1", 1) and\
                fileSearch(log_file, "Sent DMT to 1 DM services successfully", 1) and\
                fileSearch(log_file, "Sent DMT to 1 AM services successfully", 1) and\
                fileSearch(log_file, "Sent DMT to 1 SM services successfully", 1) and\
                fileSearch(log_file, "OM deployed DMT with 1 DMs", 1):
            return True
        else:
            self.log.error("OM log entries confirming DMT migration for initial 1-node cluster not found.")
            return False


# This class contains the attributes and methods to test
# whether there exists the expected DMT migration
# completion OM log entries for a 1-node cluster with a node added.
class TestTransientAddNodeDMTMigration(TestCase.FDSTestCase):
    def __init__(self, parameters=None):
        super(TestTransientAddNodeDMTMigration, self).__init__(parameters)


    def runTest(self):
        test_passed = True

        if TestCase.pyUnitTCFailure:
            self.log.warning("Skipping Case %s. stop-on-fail/failfast set and a previous test case has failed." %
                             self.__class__.__name__)
            return unittest.skip("stop-on-fail/failfast set and a previous test case has failed.")
        else:
            self.log.info("Running Case %s." % self.__class__.__name__)

        try:
            if not self.test_TransientAddNodeDMTMigration():
                test_passed = False
        except Exception as inst:
            self.log.error("Transient add node DMT migration caused exception:")
            self.log.error(traceback.format_exc())
            test_passed = False

        super(self.__class__, self).reportTestCaseResult(test_passed)

        # If there is any test fixture teardown to be done, do it here.

        if self.parameters["pyUnit"]:
            self.assertTrue(test_passed)
        else:
            return test_passed


    def test_TransientAddNodeDMTMigration(self):
        """
        Test Case:
        Attempt locate in OM's log logged statements indicating
        successful completion of the DMT migration following the
        addition of a second node to a 1-node cluster.
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]
        om_node = fdscfg.rt_om_node
        fds_dir = om_node.nd_conf_dict['fds_root']
        log_file = fds_dir + "/var/logs/om.log_0.log"

        # Some of these message will have been logged before for other reasons, so account for them.
        if fileSearch(log_file, "OM_PmAgent: will send node activate message to Node-1", 2) and\
                fileSearch(log_file, "OM_PmAgent: will send node activate message to Node-2", 1) and\
                fileSearch(log_file, "Sent DMT to 2 DM services successfully", 1) and\
                fileSearch(log_file, "Sent DMT to 1 AM services successfully", 2) and\
                fileSearch(log_file, "Sent DMT to 2 SM services successfully", 1) and\
                fileSearch(log_file, "OM deployed DMT with 2 DMs", 1):
            return True
        else:
            self.log.error("OM log entries confirming DMT migration for adding a second node to an initial 1-node cluster not found.")
            return False


# This class contains the attributes and methods to test
# whether there exists the expected DMT migration
# completion OM log entries for a 2-node cluster following a node removed.
class TestTransientRemoveNodeDMTMigration(TestCase.FDSTestCase):
    def __init__(self, parameters=None):
        super(TestTransientRemoveNodeDMTMigration, self).__init__(parameters)


    def runTest(self):
        test_passed = True

        if TestCase.pyUnitTCFailure:
            self.log.warning("Skipping Case %s. stop-on-fail/failfast set and a previous test case has failed." %
                             self.__class__.__name__)
            return unittest.skip("stop-on-fail/failfast set and a previous test case has failed.")
        else:
            self.log.info("Running Case %s." % self.__class__.__name__)

        try:
            if not self.test_TransientRemoveNodeDMTMigration():
                test_passed = False
        except Exception as inst:
            self.log.error("Transient remove node DMT migration caused exception:")
            self.log.error(traceback.format_exc())
            test_passed = False

        super(self.__class__, self).reportTestCaseResult(test_passed)

        # If there is any test fixture teardown to be done, do it here.

        if self.parameters["pyUnit"]:
            self.assertTrue(test_passed)
        else:
            return test_passed


    def test_TransientRemoveNodeDMTMigration(self):
        """
        Test Case:
        Attempt locate in OM's log logged statements indicating
        successful completion of the DMT migration following the
        removal of a second node from a 2-node cluster.
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]
        om_node = fdscfg.rt_om_node
        fds_dir = om_node.nd_conf_dict['fds_root']
        log_file = fds_dir + "/var/logs/om.log_0.log"

        # Some of these message will have been logged before for other reasons, so account for them.
        if fileSearch(log_file, "Received remove services for nodeNode-2 remove am ? true remove sm ? true remove dm ? true", 1) and\
                fileSearch(log_file, "Sent DMT to 1 DM services successfully", 2) and\
                fileSearch(log_file, "Sent DMT to 1 AM services successfully", 3) and\
                fileSearch(log_file, "Sent DMT to 1 SM services successfully", 2) and\
                fileSearch(log_file, "OM deployed DMT with 1 DMs", 2):
            return True
        else:
            self.log.error("OM log entries confirming DMT migration for removing a node from a 2-node cluster not found.")
            return False


# This class contains the attributes and methods to test
# whether there occurred successful DM Static migration
# between two nodes.
class TestVerifyDMStaticMigration(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node1=None, node2=None, volume=None):
        super(TestVerifyDMStaticMigration, self).__init__(parameters)

        self.passedNode1 = node1
        self.passedNode2 = node2
        self.passedVolume = volume


    def runTest(self):
        test_passed = True

        if TestCase.pyUnitTCFailure:
            self.log.warning("Skipping Case %s. stop-on-fail/failfast set and a previous test case has failed." %
                             self.__class__.__name__)
            return unittest.skip("stop-on-fail/failfast set and a previous test case has failed.")
        else:
            self.log.info("Running Case %s." % self.__class__.__name__)

        try:
            if not self.test_VerifyDMStaticMigration():
                test_passed = False
        except Exception as inst:
            self.log.error("DM Static migration verification caused exception:")
            self.log.error(traceback.format_exc())
            test_passed = False

        super(self.__class__, self).reportTestCaseResult(test_passed)

        # If there is any test fixture teardown to be done, do it here.

        if self.parameters["pyUnit"]:
            self.assertTrue(test_passed)
        else:
            return test_passed


    def test_VerifyDMStaticMigration(self):
        """
        Test Case:
        Verify whether DM Static migration successfully occurred
        by comparing ${FDS}/user_repo/dm_names contents of the two nodes
        for the given volume.
        """

        # Verify input.
        if (self.passedNode1 is None) or (self.passedNode2 is None) or (self.passedVolume is None):
            self.log.error("Test case construction requires parameters node1, node2 and volume.")
            raise

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        # Locate our two nodes.
        nodes = fdscfg.rt_obj.cfg_nodes
        n1 = None
        n2 = None
        for n in nodes:
            if self.passedNode1 == n.nd_conf_dict['node-name']:
                n1 = n
            if self.passedNode2 == n.nd_conf_dict['node-name']:
                n2 = n

            if (n1 is not None) and (n2 is not None):
                break

        if n1 is None:
            self.log.error("Node not found for %s." % self.passedNode1)
            raise
        if n2 is None:
            self.log.error("Node not found for %s." % self.passedNode2)
            raise

        # Locate our volume.
        volumes = fdscfg.rt_get_obj('cfg_volumes')
        v = None
        for volume in volumes:
            if self.passedVolume == volume.nd_conf_dict['vol-name']:
                v = volume

        if v is None:
            self.log.error("Volume not found for %s." % self.passedVolume)
            raise

        fds_dir1 = n1.nd_conf_dict['fds_root']
        dm_names_dir1 = fds_dir1 + "/user-repo/dm-names/" + v.nd_conf_dict['id']

        fds_dir2 = n2.nd_conf_dict['fds_root']
        dm_names_dir2 = fds_dir2 + "/user-repo/dm-names/" + v.nd_conf_dict['id']

        # Some of these message will have been logged before for other reasons, so account for them.
        if are_dir_trees_equal(dm_names_dir1, dm_names_dir2, True):  # Log differences.
            self.log.info("Equal DM Names directories %s and %s." % (dm_names_dir1, dm_names_dir2))
            return True
        else:
            self.log.error("Unequal DM Names directories %s and %s." % (dm_names_dir1, dm_names_dir2))
            return False


# This class contains the attributes and methods to test
# the consistency of SM It requires that the cluster be shut down.
class TestVerifySMConsistency(TestCase.FDSTestCase):
    def __init__(self, parameters=None):
        super(TestVerifySMConsistency, self).__init__(parameters)


    def runTest(self):
        test_passed = True

        if TestCase.pyUnitTCFailure:
            self.log.warning("Skipping Case %s. stop-on-fail/failfast set and a previous test case has failed." %
                             self.__class__.__name__)
            return unittest.skip("stop-on-fail/failfast set and a previous test case has failed.")
        else:
            self.log.info("Running Case %s." % self.__class__.__name__)

        try:
            if not self.test_VerifySMConsistency():
                test_passed = False
        except Exception as inst:
            self.log.error("SM consistency checking caused exception:")
            self.log.error(traceback.format_exc())
            test_passed = False

        super(self.__class__, self).reportTestCaseResult(test_passed)

        # If there is any test fixture teardown to be done, do it here.

        if self.parameters["pyUnit"]:
            self.assertTrue(test_passed)
        else:
            return test_passed


    def test_VerifySMConsistency(self):
        """
        Test Case:
        Attempt to run the the .../bin/smchk utility to confirm SM consistency.

        WARNING: This implementation assumes that the cluster is down.
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]
        om_node = fdscfg.rt_om_node
        bin_dir = fdscfg.rt_env.get_bin_dir(debug=False)
        log_dir = fdscfg.rt_env.get_log_dir()
        fds_dir = om_node.nd_conf_dict['fds_root']

        self.log.info("Check SM consistency.")

        status = om_node.nd_agent.exec_wait('bash -c \"(nohup %s/smchk --fds-root=%s --full-check > %s/smchk.out 2>&1 &) \"' %
                                      (bin_dir, fds_dir, log_dir))

        if status != 0:
            self.log.error("SM consistency checking using node %s returned status %d." %
                           (om_node.nd_conf_dict['fds_node'], status))
            return False

        return True

if __name__ == '__main__':
    TestCase.FDSTestCase.fdsGetCmdLineConfigs(sys.argv)
    unittest.main()