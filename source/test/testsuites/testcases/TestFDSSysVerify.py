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
from fabric.contrib.files import *
from fabric.context_managers import cd
from fdslib.TestUtils import disconnect_fabric
from fdslib.TestUtils import connect_fabric


def fileSearch(searchFile, searchString, occurrences, sftp=None):
    """
    Search the given file for the given search string
    verifying whether it occurs the specified number
    of times.
    """
    log = logging.getLogger('TestFDSSysVerify' + '.' + 'fileSearch')

    # Verify the file exists.
    if sftp is None:
        if not os.path.isfile(searchFile):
            log.error("File %s is not found." % (searchFile))
            return False
    else:
        lstatout = str(sftp.lstat(searchFile)).split()[0]
        if 'd' in lstatout:
            log.error("%s is a directory." % (searchFile))
            return False

    if sftp is None:
        f = open(searchFile, "r")
        searchLines = f.readlines()
        log.debug("Read {} lines from {}. Found {} as first line and {} as last line.".format(len(searchLines),
                                                                                              searchFile,
                                                                                              searchLines[0],
                                                                                              searchLines[-1]))
        f.close()
    else:
        f = sftp.file(searchFile, "r", -1)
        data = f.read()
        searchLines = data.split('\n')

    occur = 0
    log.debug("Searching for exactly %d occurrence(s) of string [%s]." % (occurrences, searchString))
    for line in searchLines:
        if searchString in line:
            occur = occur + 1
            log.debug("Found occurrence %d: %s." % (occur, line))

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


def canonMatch(canon, fileToCheck, adjustLines = False, moreVersions=False):
    """
    Test whether the fileToCheck file matches the canon file.
    This is a regular expression check where the canon file may
    contain regular expression patterns to aid in custom compares.

    If adjustLines is set True, then the number of lines in
    the canon determines the first number f line in the file we
    check.
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
        if (adjustLines) and (len(canonLines) > len(linesToCheck)):
            log.warn("Canon mis-match on line count: %s line count: %s. %s line count: %s." %
                      (canon, len(canonLines), fileToCheck, len(fileToCheck)))
            return False

    idx = 0
    for canonLine in canonLines:
        if re.match(canonLine.encode('string-escape'), linesToCheck[idx]) is None:
            if moreVersions:
                log.info("No match.")
            else:
                log.warn("File %s, differs from canon file, %s, at line %d." %
                          (fileToCheck, canon, idx + 1))
                log.warn("File line:")
                log.warn(linesToCheck[idx])
                log.warn("Canon line:")
                log.warn(canonLine)
            return False
        else:
            idx += 1

    return True


def areTokenFilesUpdated_AWS(self, node_ip, dev_dir, media_type):
    connect_fabric(self,node_ip)

    with cd(dev_dir):
        drives_string = run('ls')
        drives = drives_string.split()
        for drive in drives:
            if fnmatch.fnmatch(drive, media_type + "-*"):
                with cd(dev_dir + drive):
                    drive_files_str = run('ls')
                    drive_files = drive_files_str.split()
                    for drive_file in drive_files:
                        if fnmatch.fnmatch(drive_file, "tokenFile_*_*"):
                            with cd(dev_dir + drive):
                                cmd = "du -b %s | cut -f1" % (drive_file)
                                token_file_size = sudo(cmd)
                                token_file_size = int(token_file_size)
                                if token_file_size > 0:
                                    disconnect_fabric()
                                    return True
    disconnect_fabric()
    return False


def areTokenFilesUpdated(self, dev_dir, media_type):
    fdscfg = self.parameters["fdscfg"]
    nodes = fdscfg.rt_obj.cfg_nodes
    for node in nodes:
        if node.nd_conf_dict['node-name'] == self.passedNode:
            node_ip = node.nd_conf_dict['ip']
            break
    if node_ip is None:
        raise Exception
    if node_ip == 'localhost':
        drives = os.listdir(dev_dir)
        for drive in drives:
            if fnmatch.fnmatch(drive, media_type + "-*"):
                drive_files = os.listdir(dev_dir + drive)
                for drive_file in drive_files:
                    if fnmatch.fnmatch(drive_file, "tokenFile_*_*"):
                        token_file_size = os.stat(dev_dir + drive + "/" + drive_file).st_size
                        if token_file_size > 0:
                            return True
    else:
        return areTokenFilesUpdated_AWS(self, node_ip, dev_dir, media_type)
    return False


# This class contains the attributes and methods to test
# whether there occurred successful DM Static migration
# between two nodes using directory/file comparisons.
class TestVerifyDMStaticMigration_byFileCompare(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node1=None, node2=None, volume=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_VerifyDMStaticMigration_byFileCompare,
                                             "DM Static migration verification")

        self.passedNode1 = node1
        self.passedNode2 = node2
        self.passedVolume = volume

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


class TestDMChecker(TestCase.FDSTestCase):
    def __init__(self, parameters=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_DMChecker,
                                             "DM Static migration verification")

    def test_DMChecker(self):
        fdscfg = self.parameters["fdscfg"]
        bin_dir = fdscfg.rt_env.get_bin_dir(debug=False)
        nodes = fdscfg.rt_obj.cfg_nodes
        n1 = nodes[0]
        status, stdout = n1.nd_agent.exec_wait('bash -c \"(nohup %s/DmChecker) \"' %
                                               (bin_dir), return_stdin=True)

        if status != 0:
            self.log.error("DM Static Migration failed")
            return False
        return True


# This class contains the attributes and methods to test
# whether there occurred successful SM Static migration
# between two nodes using the SM Check utility.
class TestVerifySMStaticMigration(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node1=None, node2=None, skip=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_VerifySMStaticMigration,
                                             "SM Static Migration checking")

        self.passedNode1 = node1
        self.passedNode2 = node2
        self.skip = skip  # Does the caller wish to execute even if previous test cases have failed?

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
        status, stdout1 = n1.nd_agent.exec_wait('bash -c \"(nohup %s/smchk --fds-root=%s --full-check -o -a) \"' %
                                                (bin_dir, fds_dir1), return_stdin=True)

        if status != 0:
            self.log.error("SM Static Migration checking using node %s returned status %d." %
                           (n1.nd_conf_dict['node-name'], status))
            return False

        # Now capture stdout from smchk for node2.
        status, stdout2 = n1.nd_agent.exec_wait('bash -c \"(nohup %s/smchk --fds-root=%s --full-check -o -a) \"' %
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
# whether SM migration successfully migrated object metadata
# between two nodes using the SM Check utility.
# It will not check that the objects are valid, but
# that the metadata and ref counts are consistent.
# For validation of objects, use TestVerifySMStaticMigration
class TestVerifySMMetaMigration(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node1=None, node2=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_VerifySMMetaMigration,
                                             "SM Metadata Migration checking")

        self.passedNode1 = node1
        self.passedNode2 = node2

    def test_VerifySMMetaMigration(self):
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
        status, stdout1 = n1.nd_agent.exec_wait('bash -c \"(nohup %s/smchk --fds-root=%s --list-active-metadata) \"' %
                                                (bin_dir, fds_dir1), return_stdin=True)

        if status != 0:
            self.log.error("SM Static Migration checking using node %s returned status %d." %
                           (n1.nd_conf_dict['node-name'], status))
            return False

        # Now capture stdout from smchk for node2.
        status, stdout2 = n1.nd_agent.exec_wait('bash -c \"(nohup %s/smchk --fds-root=%s --list-active-metadata) \"' %
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
    def __init__(self, parameters=None, node=None, service=None, logentry=None, occurrences=None, maxwait=None,
                 atleastcount=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_WaitForLog,
                                             "Waiting for a log entry")

        self.passedNode = node
        self.passedService = service
        self.passedLogentry = logentry
        self.passedOccurrences = occurrences
        self.passedMaxwait = maxwait
        self.atleastcount = atleastcount

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

        if self.atleastcount is not None:
            self.log.info("Looking in %s's %s logs for entry '%s' to occur at least %s. Waiting for up to %s seconds." %
                          (self.passedNode.nd_conf_dict['node-name'], self.passedService,
                           self.passedLogentry, self.atleastcount, self.passedMaxwait))
        else:
            self.log.info(
                "Looking in node %s's %s logs for entry '%s' to occur %s times. Waiting for up to %s seconds." %
                (self.passedNode.nd_conf_dict['node-name'], self.passedService,
                 self.passedLogentry, self.passedOccurrences, self.passedMaxwait))

        # We'll check for the specified log entry every 10 seconds until we
        # either find what we're looking for or timeout while looking.
        maxLooks = self.passedMaxwait / 10
        occurrencesFound = 0
        for i in range(1, maxLooks):
            occurrencesFound = 0
            sftp = None
            if self.passedNode.nd_local:
                log_files = os.listdir(fds_dir + "/var/logs")
                self.log.debug("Found {} files during os.listdir. Files are {}".format(len(log_files), log_files))
            else:
                sftp = self.passedNode.nd_agent.env_ssh_clnt.open_sftp()
                log_files = sftp.listdir(fds_dir + "/var/logs")

            for log_file in log_files:
                self.log.debug("Currently checking if {} is a log file we're interested in...".format(log_file))
                if fnmatch.fnmatch(log_file, self.passedService + ".log_*.log"):
                    self.log.debug("{} is a log file of interest.".format(log_file))
                    found, occurrences = fileSearch(fds_dir + "/var/logs/" + log_file, self.passedLogentry,
                                                    self.passedOccurrences, sftp)
                    occurrencesFound += occurrences
                    if self.atleastcount is not None and occurrencesFound > self.atleastcount:
                        # Found minimum count of log entries, we're good to go
                        break

                if occurrencesFound > self.passedOccurrences:
                    # Found too many.
                    break

            if sftp is not None:
                sftp.close()

            if ((self.atleastcount is not None) and (occurrencesFound > self.atleastcount)):
                break
            elif occurrencesFound >= self.passedOccurrences:
                # Saw what we were looking for or found too many.
                break
            else:
                time.sleep(10)
                self.log.info("Looking ...")

        if ((self.atleastcount is not None) and (occurrencesFound > self.atleastcount)):
            self.log.info("Log entry found at least %s." % self.atleastcount)
            return True
        else:
            self.log.info("Log entry found %s times." % occurrencesFound)

        if occurrencesFound != self.passedOccurrences:
            self.log.error("Expected %s occurrences." % self.passedOccurrences)
            return False
        else:
            return True


# This class checks for updates in the token files of ssd drives attached to the cluster
class TestCheckSSDTokenFiles(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_CheckSSDTokenFiles,
                                             "Checking SSD token files")
        self.passedNode = node

    def test_CheckSSDTokenFiles(self):
        """
        Test Case:
        Check SSD token files to test whether then writes for a volume created with --media-type as SSD
        are indeed going to SSD disks.
        """

        # We must have all our parameters supplied.
        if (self.passedNode is None):
            self.log.error("Parameter missing values.")
            raise Exception

        fds_dir = "/fds"
        if self.parameters['install'] is not True:
            fds_dir = fds_dir + '/' + self.passedNode

        self.log.info("Looking in ssd devices of node %s for token files. And fds_dir is %s"
                      % (self.passedNode, fds_dir))
        dev_dir = fds_dir + "/dev/"
        return areTokenFilesUpdated(self, dev_dir, "ssd")


# This class checks for updates in the token files of hard disk drives attached to the cluster
class TestCheckHDDTokenFiles(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_CheckHDDTokenFiles,
                                             "Checking HDD token files")
        self.passedNode = node

    def test_CheckHDDTokenFiles(self):
        """
        Test Case:
        Check HDD token files to test whether then writes for a volume created with --media-type as HDD
        are indeed going to HDD disks.
        """

        # We must have all our parameters supplied.
        if (self.passedNode is None):
            self.log.error("Parameter missing values.")
            raise Exception

        fds_dir = "/fds"
        if self.parameters['install'] is not True:
            fds_dir = fds_dir + '/' + self.passedNode

        self.log.info("Looking in hdd devices of node %s for token files. And fds_dir is %s"
                      % (self.passedNode, fds_dir))
        dev_dir = fds_dir + "/dev/"
        return areTokenFilesUpdated(self, dev_dir, "hdd")


# This class checks for updates in the token files of both ssd and hard disk drives attached to the cluster
class TestCheckHybridTokenFiles(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_CheckHybridTokenFiles,
                                             "Checking token files for SSDs and HDDs")
        self.passedNode = node

    def test_CheckHybridTokenFiles(self):
        """
        Test Case:
        Check token files for ssd and hdd to test whether then writes for a volume created with
        --media-type as hybrid are indeed going to SSD and HDD.
        """

        # We must have all our parameters supplied.
        if (self.passedNode is None):
            self.log.error("Parameter missing values.")
            raise Exception

        fds_dir = "/fds"
        if self.parameters['install'] is not True:
            fds_dir = fds_dir + '/' + self.passedNode

        self.log.info("Looking in ssd and hdd devices of node %s for token files. And fds_dir is %s"
                      % (self.passedNode, fds_dir))
        dev_dir = fds_dir + "/dev/"
        return (areTokenFilesUpdated(self, dev_dir, "ssd") and areTokenFilesUpdated(self, dev_dir, "hdd"))


# This class enables and runs the garbage collector on all the SMs in the cluster
class TestRunScavenger(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_RunScavenger,
                                             "Run scavenger(garbage collector)")

    def test_RunScavenger(self):
        """
        Test Case:
        Run garbage collector for all SMs in the cluster.
        """

        fdscfg = self.parameters["fdscfg"]
        om_node = fdscfg.rt_om_node

        self.log.info("Enabling garbage collector")
        status = om_node.nd_agent.exec_wait('bash -c \"(./fdsconsole.py domain setScavenger local enable) \"',
                                            fds_tools=True)
        status = om_node.nd_agent.exec_wait('bash -c \"(./fdsconsole.py domain setScavenger local start) \"',
                                            fds_tools=True)
        if status != 0:
            self.log.error("Starting garbage collector failed")
            return False
        else:
            self.log.info("Starting garbage collector succeeded")
            return True


# This class contains the attributes and methods to test
# whether the specified file matches the specified canon file
# in a regular expression compare.
class TestCanonMatch(TestCase.FDSTestCase):
    def __init__(self, parameters=None, canon=None, fileToCheck=None, adjustLines=False):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_CanonMatch,
                                             "Performing a canon match")

        self.passedCanon = canon
        self.passedFileToCheck = fileToCheck
        self.passedAdjustLines = adjustLines

    def test_CanonMatch(self):
        """
        Test Case:
        Report the results of a regular expression compare between
        the canon file and the file to be checked.
        """

        # We must have our canon and fileToCheck parameters supplied.
        if (self.passedCanon is None) or \
                (self.passedFileToCheck is None):
            self.log.error("Some parameters missing values.")
            raise Exception

        fdscfg = self.parameters["fdscfg"]
        canonDir = fdscfg.rt_env.get_fds_source() + "test/testsuites/canons/"

        # If we don't see the canon file, it may be that there are multiple
        # versions that we need to check. Different versions of a canon will
        # all have the same file name except that they will have a final
        # LLQ starting with "v" and followed by a number. For example:
        # 3.stat_min_log.v1
        # 3.stat_min_log.v2
        # are two different versions of canon "3.stat_min_log".
        #
        # If we have different version, we'll check each in turn until we
        # either find a match (success!) or find that none match (fail!).
        if not os.path.isfile(canonDir + self.passedCanon):
            canonFileDir, canonFilePrefix = os.path.split(canonDir + self.passedCanon)
            listOfCanonDirFiles = os.listdir(canonFileDir)

            found = False
            listOfCanons = []
            i = 0
            for file in listOfCanonDirFiles:
                if (re.search(canonFilePrefix + '[.]v[0-9]', file)):
                    found = True
                    i += 1
                    listOfCanons.append(file)

            if not found:
                self.log.error("Canon file %s is not found." % (self.passedCanon))
                return False

            self.log.info("{0} versions for canon {1}.".format(i, self.passedCanon))

            for file in listOfCanons:
                i -= 1

                self.log.info("Comparing file %s to canon %s." %
                              (self.passedFileToCheck, canonFileDir + os.path.sep + file))

                if canonMatch(canonFileDir + os.path.sep + file, self.passedFileToCheck, adjustLines=self.passedAdjustLines, moreVersions=(i > 0)):
                    return True

            return False
        else:
            # Just check the one file.
            self.log.info("Comparing file %s to canon %s." %
                          (self.passedFileToCheck, canonDir + self.passedCanon))

            if canonMatch(canonDir + self.passedCanon, self.passedFileToCheck, adjustLines=self.passedAdjustLines):
                return True
            else:
                self.log.error("Canon match failed.")
                return False


if __name__ == '__main__':
    TestCase.FDSTestCase.fdsGetCmdLineConfigs(sys.argv)
    unittest.main()
