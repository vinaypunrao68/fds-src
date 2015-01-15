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
import fnmatch
import time
import re

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
        return True, occur
    else:
        return False, occur


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


def canonMatch(canon, fileToCheck):
    """
    Test whether the fileToCheck file matches the canon file.
    This is a regular expression check where the canon file may
    contain regular expression patterns to aid in custom compares.
    """
    log = logging.getLogger('TestFDSSysVerify' + '.' + 'canonMatch')

    # Verify the files exists.
    if not os.path.isfile(canon):
        log.error("File %s is not found." % (canon))
        return False
    if not os.path.isfile(fileToCheck):
        log.error("File %s is not found." % (fileToCheck))
        return False

    # Open both files then go through line by line performing
    # a regular expression match. The first mis-match results
    # in a failed match.
    c = open(canon, "r")
    canonLines = c.readlines()
    c.close()

    f = open(fileToCheck, "r")
    linesToCheck = f.readlines()
    f.close()

    if len(canonLines) != len(linesToCheck):
        log.error("Canon mis-match on line count: %s line count: %s. %s line count: %s." %
                  (canon, len(canonLines), fileToCheck, len(fileToCheck)))
        return False

    idx = 0
    for canonLine in canonLines:
        if re.match(canonLine.encode('string-escape'), linesToCheck[idx]) is None:
            log.error("File %s, differs from canon file, %s, at line %d." %
                      (fileToCheck, canon, idx+1))
            log.error("File line:")
            log.error(linesToCheck[idx])
            log.error("Canon line:")
            log.error(canonLine)
            return False
        else:
            idx += 1

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
# between two nodes using directory/file comparisons.
class TestVerifyDMStaticMigration_byFileCompare(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node1=None, node2=None, volume=None):
        super(TestVerifyDMStaticMigration_byFileCompare, self).__init__(parameters)

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
            if not self.test_VerifyDMStaticMigration_byFileCompare():
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


    def test_VerifyDMStaticMigration_byFileCompare(self):
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

        # Compare the DM directories and files between the two nodes for the given volume.
        if are_dir_trees_equal(dm_names_dir1, dm_names_dir2, True):  # Log differences.
            self.log.info("Equal DM Names directories %s and %s." % (dm_names_dir1, dm_names_dir2))
            return True
        else:
            self.log.error("Unequal DM Names directories %s and %s." % (dm_names_dir1, dm_names_dir2))
            return False


# This class contains the attributes and methods to test
# whether there occurred successful DM Static migration
# between two nodes using the DM Check utility.
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
            self.log.error("DM Static Migration checking caused exception:")
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
        by comparing the output of the DM Check utility run for each node.
        """

        # Verify input.
        if (self.passedNode1 is None) or (self.passedNode2 is None) or (self.passedVolume is None):
            self.log.error("Test case construction requires parameters node1, node2 and volume.")
            raise Exception

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]
        bin_dir = fdscfg.rt_env.get_bin_dir(debug=False)

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

        fds_dir2 = n2.nd_conf_dict['fds_root']

        # Capture stdout from dmchk for node1.
        #
        # Parameter return_stdin is set to return stdout. ... Don't ask me!
        status, stdout1 = n1.nd_agent.exec_wait('bash -c \"(nohup %s/dmchk --fds-root=%s -l %s) \"' %
                                                (bin_dir, fds_dir1, v.nd_conf_dict['id']), return_stdin=True)

        if status != 0:
            self.log.error("DM Static Migration checking using node %s returned status %d." %
                           (n1.nd_conf_dict['node-name'], status))
            return False

        # Now capture stdout from dmchk for node2.
        status, stdout2 = n1.nd_agent.exec_wait('bash -c \"(nohup %s/dmchk --fds-root=%s -l %s) \"' %
                                                (bin_dir, fds_dir2, v.nd_conf_dict['id']), return_stdin=True)

        if status != 0:
            self.log.error("DM Static Migration checking using node %s returned status %d." %
                           (n2.nd_conf_dict['node-name'], status))
            return False

        # Let's compare.
        if stdout1 != stdout2:
            self.log.error("DM Static Migration checking showed differences.")
            self.log.error("Node %s shows: \n%s" %
                           (n1.nd_conf_dict['node-name'], stdout1))
            self.log.error("Node %s shows: \n%s" %
                           (n2.nd_conf_dict['node-name'], stdout2))
            return False
        else:
            self.log.info("DM Static Migration checking showed match between nodes %s and %s." %
                          (n1.nd_conf_dict['node-name'], n2.nd_conf_dict['node-name']))
            self.log.info("Both nodes show: \n%s" % stdout1)

        return True


# This class contains the attributes and methods to test
# whether there occurred successful SM Static migration
# between two nodes using the SM Check utility.
class TestVerifySMStaticMigration(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node1=None, node2=None, skip=None):
        super(TestVerifySMStaticMigration, self).__init__(parameters)

        self.passedNode1 = node1
        self.passedNode2 = node2
        self.skip = skip  # Does the caller wish to execute even if previous test cases have failed?


    def runTest(self):
        test_passed = True

        # The caller may tell us not to skip this test case.
        if (TestCase.pyUnitTCFailure) and (self.skip is None):
            self.log.warning("Skipping Case %s. stop-on-fail/failfast set and a previous test case has failed." %
                             self.__class__.__name__)
            return unittest.skip("stop-on-fail/failfast set and a previous test case has failed.")
        else:
            self.log.info("Running Case %s." % self.__class__.__name__)

        try:
            if not self.test_VerifySMStaticMigration():
                test_passed = False
        except Exception as inst:
            self.log.error("SM Static Migration checking caused exception:")
            self.log.error(traceback.format_exc())
            test_passed = False

        super(self.__class__, self).reportTestCaseResult(test_passed)

        # If there is any test fixture teardown to be done, do it here.

        if self.parameters["pyUnit"]:
            self.assertTrue(test_passed)
        else:
            return test_passed


    def test_VerifySMStaticMigration(self):
        """
        Test Case:
        Verify whether SM Static migration successfully occurred
        by comparing the output of the SM Check utility run for each node.
        """

        # Verify input.
        if (self.passedNode1 is None) or (self.passedNode2 is None):
            self.log.error("Test case construction requires parameters node1 and node2.")
            raise Exception

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]
        bin_dir = fdscfg.rt_env.get_bin_dir(debug=False)

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

        fds_dir1 = n1.nd_conf_dict['fds_root']

        fds_dir2 = n2.nd_conf_dict['fds_root']

        # Capture stdout from smchk for node1.
        #
        # Parameter return_stdin is set to return stdout. ... Don't ask me!
        status, stdout1 = n1.nd_agent.exec_wait('bash -c \"(nohup %s/smchk --fds-root=%s --full-check) \"' %
                                                (bin_dir, fds_dir1), return_stdin=True)

        if status != 0:
            self.log.error("SM Static Migration checking using node %s returned status %d." %
                           (n1.nd_conf_dict['node-name'], status))
            return False

        # Now capture stdout from smchk for node2.
        status, stdout2 = n1.nd_agent.exec_wait('bash -c \"(nohup %s/smchk --fds-root=%s --full-check) \"' %
                                                (bin_dir, fds_dir2), return_stdin=True)

        if status != 0:
            self.log.error("SM Static Migration checking using node %s returned status %d." %
                           (n2.nd_conf_dict['node-name'], status))
            return False

        # Let's compare.
        if stdout1 != stdout2:
            self.log.error("SM Static Migration checking showed differences.")
            self.log.error("Node %s shows: \n%s" %
                           (n1.nd_conf_dict['node-name'], stdout1))
            self.log.error("Node %s shows: \n%s" %
                           (n2.nd_conf_dict['node-name'], stdout2))
            return False
        else:
            self.log.info("SM Static Migration checking showed match between nodes %s and %s." %
                          (n1.nd_conf_dict['node-name'], n2.nd_conf_dict['node-name']))
            self.log.info("Both nodes show: \n%s" % stdout1)

        return True


# This class contains the attributes and methods to test
# whether the specified log entry can be located the sepcified number of times
# in the specified log before expiration of the specified time.
class TestWaitForLog(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None, service=None, logentry=None, occurrences=None, maxwait=None):
        super(self.__class__, self).__init__(parameters)

        self.passedNode = node
        self.passedService = service
        self.passedLogentry = logentry
        self.passedOccurrences = occurrences
        self.passedMaxwait = maxwait

    def runTest(self):
        test_passed = True

        if TestCase.pyUnitTCFailure:
            self.log.warning("Skipping Case %s. stop-on-fail/failfast set and a previous test case has failed." %
                             self.__class__.__name__)
            return unittest.skip("stop-on-fail/failfast set and a previous test case has failed.")
        else:
            self.log.info("Running Case %s." % self.__class__.__name__)

        try:
            if not self.test_WaitForLog():
                test_passed = False
        except Exception as inst:
            self.log.error("Waiting for a log entry caused exception:")
            self.log.error(traceback.format_exc())
            test_passed = False

        super(self.__class__, self).reportTestCaseResult(test_passed)

        # If there is any test fixture teardown to be done, do it here.

        if self.parameters["pyUnit"]:
            self.assertTrue(test_passed)
        else:
            return test_passed


    def test_WaitForLog(self):
        """
        Test Case:
        Attempt to locate in the specified log the sought after entry.
        """

        # We must have all our parameters supplied.
        if (self.passedNode is None) or \
                (self.passedService is None) or \
                (self.passedLogentry is None) or \
                (self.passedOccurrences is None) or \
                (self.passedMaxwait is None):
            self.log.error("Some parameters missing values.")
            raise Exception

        fds_dir = self.passedNode.nd_conf_dict['fds_root']

        self.log.info("Looking in node %s's %s logs for entry '%s' to occur %s times. Waiting for up to %s seconds." %
                       (self.passedNode.nd_conf_dict['node-name'], self.passedService,
                        self.passedLogentry, self.passedOccurrences, self.passedMaxwait))

        # We'll check for the specified log entry every 10 seconds until we
        # either find what we're looking for or timeout while looking.
        maxLooks = self.passedMaxwait / 10
        occurrencesFound = 0
        for i in range(1, maxLooks):
            occurrencesFound = 0
            for log_file in os.listdir(fds_dir + "/var/logs"):
                if fnmatch.fnmatch(log_file, self.passedService + ".log_*.log"):
                    found, occurrences = fileSearch(fds_dir + "/var/logs/" + log_file, self.passedLogentry,
                                                    self.passedOccurrences)
                    occurrencesFound += occurrences

            if occurrencesFound == self.passedOccurrences:
                # Saw what we were looking for.
                break
            else:
                time.sleep(10)
                self.log.info("Looking ...")

        self.log.info("Log entry found %s times." % occurrencesFound)

        if occurrencesFound != self.passedOccurrences:
            self.log.error("Expected %s occurrences." % self.passedOccurrences)
            return False
        else:
            return True


# This class contains the attributes and methods to test
# whether he specified file matches the specified canon file
# in a regular expression compare.
class TestCanonMatch(TestCase.FDSTestCase):
    def __init__(self, parameters=None, canon=None, fileToCheck=None):
        super(self.__class__, self).__init__(parameters)

        self.passedCanon = canon
        self.passedFileToCheck = fileToCheck

    def runTest(self):
        test_passed = True

        if TestCase.pyUnitTCFailure:
            self.log.warning("Skipping Case %s. stop-on-fail/failfast set and a previous test case has failed." %
                             self.__class__.__name__)
            return unittest.skip("stop-on-fail/failfast set and a previous test case has failed.")
        else:
            self.log.info("Running Case %s." % self.__class__.__name__)

        try:
            if not self.test_CanonMatch():
                test_passed = False
        except Exception as inst:
            self.log.error("Performing a canon match caused exception:")
            self.log.error(traceback.format_exc())
            test_passed = False

        super(self.__class__, self).reportTestCaseResult(test_passed)

        # If there is any test fixture teardown to be done, do it here.

        if self.parameters["pyUnit"]:
            self.assertTrue(test_passed)
        else:
            return test_passed


    def test_CanonMatch(self):
        """
        Test Case:
        Report the results of a regular expression compare between
        the canon file and the file to be checked.
        """

        # We must have all our parameters supplied.
        if (self.passedCanon is None) or \
                (self.passedFileToCheck is None):
            self.log.error("Some parameters missing values.")
            raise Exception

        fdscfg = self.parameters["fdscfg"]
        canonDir = fdscfg.rt_env.get_fds_source() + "test/testsuites/canons/"

        self.log.info("Comparing file %s to canon %s." %
                      (self.passedFileToCheck, canonDir + self.passedCanon))

        if canonMatch(canonDir + self.passedCanon, self.passedFileToCheck):
            return True
        else:
            self.log.error("Canon match failed.")
            return False


if __name__ == '__main__':
    TestCase.FDSTestCase.fdsGetCmdLineConfigs(sys.argv)
    unittest.main()