#!/usr/bin/python
#
# Copyright 2014 by Formation Data Systems, Inc.
#

# FDS test-case pattern requirements.
import unittest
import TestCase

# Module-specific requirements
import sys
import os
import time
import tempfile
import TestFDSSysMgt
from fdslib.TestUtils import findNodeFromInv
import subprocess

# This class contains attributes and methods to test
# creating an FDS installation from a development environment.
class TestFDSInstall(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_FDSInstall,
                                             "FDS installation")

        self.passedNode = node

    def test_FDSInstall(self):
        """
        Test Case:
        Attempt to install FDS.
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            # If we were passed a node, use it and get out. Otherwise,
            # we spin through all defined nodes setting them up.
            if self.passedNode is not None:
                n = self.passedNode

            # Are we installing from a deployable package or a development environment?
            if n.nd_agent.env_install:
                if not self.parameters["ansible_install_done"]:
                    # TODO(Greg): We accommodate the Ansible deployment scripts which expect to handle
                    # all package installation nodes at once.
                    installResult = self.fromPkg(n)
                    self.parameters["ansible_install_done"] = True
                else:
                    installResult = True
            else:
                installResult = self.fromDev(n)

            if installResult == False:
                self.log.error("FDS installation on node %s failed." %
                               (n.nd_conf_dict['node-name']))
                return False
            elif self.passedNode is not None:
                # If we were passed a specific node, just take care
                # of that one and exit.

                # Actually, before we return, given how Ansible works today (3/2/2915), let's
                # spin though the list of nodes and force the Ansible installation now if any
                # nodes require it.
                for n2 in nodes:
                    if n2.nd_agent.env_install:
                        if not self.parameters["ansible_install_done"]:
                            installResult = self.fromPkg(n2)

                            if installResult == False:
                                self.log.error("FDS installation on node %s failed." %
                                               (n2.nd_conf_dict['node-name']))
                                return False

                            self.parameters["ansible_install_done"] = True

                return True

        return True

    def fromDev(self, node):
        """
        Test Case:
        Attempt to create the FDS installation directory structure
        for an installation based upon a development environment.
        """
        self.log.info("FDS development installation for node %s." %
                      (node.nd_conf_dict['node-name']))

        # Check to see if the FDS root directory is already there.
        fds_dir = node.nd_conf_dict['fds_root']
        if not os.path.exists(fds_dir):
            # Not there, try to create it.
            self.log.info("FDS installation directory, %s, nonexistent on node %s. Attempting to create." %
                          (fds_dir, node.nd_conf_dict['node-name']))
            status = node.nd_agent.exec_wait('mkdir -p ' + fds_dir)
            if status != 0:
                self.log.error("FDS installation directory creation on node %s returned status %d." %
                               (node.nd_conf_dict['node-name'], status))
                return False
        else:
            self.log.warn("FDS installation directory, %s, exists on node %s." %
                          (fds_dir, node.nd_conf_dict['node-name']))

        # Populate product memcheck directory if necessary.
        # It should be necessary since we did not do a package install.
        dest_config_dir = fds_dir + "/memcheck"
        # Check to see if the etc directory is already there.
        if not os.path.exists(dest_config_dir):
            # Not there, try to create it.
            self.log.info("FDS memcheck directory, %s, nonexistent on node %s. Attempting to create." %
                          (dest_config_dir, node.nd_conf_dict['node-name']))
            status = node.nd_agent.exec_wait('mkdir -p ' + dest_config_dir)
            if status != 0:
                self.log.error("FDS memcheck directory creation on node %s returned status %d." %
                               (node.nd_conf_dict['node-name'], status))
                return False
        else:
            self.log.warn("FDS memcheck directory, %s, exists on node %s." %
                          (dest_config_dir, node.nd_conf_dict['node-name']))
        # Load the memcheck suppressions from the devevelopment environment.
        src_config_dir = node.nd_agent.get_memcheck_dir()
        status = node.nd_agent.exec_wait('cp -rf ' + src_config_dir + ' ' + fds_dir)
        if status != 0:
            self.log.error("FDS memcheck directory population on node %s returned status %d." %
                           (node.nd_conf_dict['node-name'], status))
            return False

        # Populate product configuration directory if necessary.
        # It should be necessary since we did not do a package install.
        dest_config_dir = fds_dir + "/etc"
        # Check to see if the etc directory is already there.
        if not os.path.exists(dest_config_dir):
            # Not there, try to create it.
            self.log.info("FDS configuration directory, %s, nonexistent on node %s. Attempting to create." %
                          (dest_config_dir, node.nd_conf_dict['node-name']))
            status = node.nd_agent.exec_wait('mkdir -p ' + dest_config_dir)
            if status != 0:
                self.log.error("FDS configuration directory creation on node %s returned status %d." %
                               (node.nd_conf_dict['node-name'], status))
                return False
        else:
            self.log.warn("FDS configuration directory, %s, exists on node %s." %
                          (dest_config_dir, node.nd_conf_dict['node-name']))

        # Load the product configuration directory from the development environment.
        src_config_dir = node.nd_agent.get_config_dir()
        status = node.nd_agent.exec_wait('cp -rf ' + src_config_dir + ' ' + fds_dir)
        if status != 0:
            self.log.error("FDS configuration directory population on node %s returned status %d." %
                           (node.nd_conf_dict['node-name'], status))
            return False

        # Make sure the platform config file specifies the correct PM port given
        # in the test config file.
        if 'fds_port' in node.nd_conf_dict:
            port = node.nd_conf_dict['fds_port']
        else:
            self.log.error("FDS platform configuration file is missing 'platform_port = ' config")
            return False

        node.nd_agent.exec_wait('rm {}/platform.conf '.format(dest_config_dir))

        status = node.nd_agent.exec_wait('sed -e "s/ platform_port = 7000/ platform_port = {}/g" '
                                         '-e "1,$w {}/platform.conf" '
                                         '{}/platform.conf '.format(port, dest_config_dir, src_config_dir))

        if status != 0:
            self.log.error("FDS platform configuration file modification failed.")
            return False

        # Populate product lib directory if necessary.
        # It should be necessary since we did not do a package install.
        dest_lib_dir = fds_dir + "/lib"
        # Check to see if the etc directory is already there.
        if not os.path.exists(dest_lib_dir):
            # Not there.
            self.log.info("FDS lib directory, %s, nonexistent on node %s. Attempting to create it via soft link." %
                          (dest_lib_dir, node.nd_conf_dict['node-name']))
            src_lib_dir = node.nd_agent.get_lib_dir(debug=False)
            status = node.nd_agent.exec_wait('ln -s ' + src_lib_dir + ' ' + dest_lib_dir)
            if status != 0:
                self.log.error("FDS lib directory creation on node %s returned status %d." %
                               (node.nd_conf_dict['node-name'], status))
                return False
        else:
            self.log.warn("FDS lib directory, %s, exists on node %s." %
                          (dest_lib_dir, node.nd_conf_dict['node-name']))

        return True

    def fromPkg(self, node):
        """
        Test Case:
        Attempt to create the FDS installation directory structure
        for an installation based upon a deployable package.
        """
        self.log.info("FDS package installation for node %s." %
                      (node.nd_conf_dict['node-name']))

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        # Right now just one install section per Scenario config.
        installs = fdscfg.rt_get_obj('cfg_install')
        install = installs[0]

        # Generate the inventory file.
        print os.getcwd()
        templateDir = fdscfg.rt_env.get_fds_source() + "test/testsuites/templates/"
        tempInventory = tempfile.NamedTemporaryFile()

        # Place the IP address or DNS name of the host machine.
        # Please the IP address or DNS name of the OM node.
        om_node = fdscfg.rt_om_node
        localhost = fdscfg.rt_get_obj('cfg_localhost')

        # TODO(GREG): Until Ansible can install a single node at a time,
        # we generate a new-line delimited list of machine names to replace all instances
        # of <install_node> in the template with a list of all the nodes defined for the
        # test. We only run Ansible once, therefore, to install FDS on all listed machines rather
        # than for just the specified node.
        inventory_machine_list = ''
        for n in fdscfg.rt_obj.cfg_nodes:
            if n.nd_agent.env_install or (om_node.nd_conf_dict['node-name'] == n.nd_conf_dict['node-name']):
                inventory_machine_list = inventory_machine_list + n.nd_conf_dict['ip'] + '\\' + '\n'

        status = localhost.nd_agent.exec_wait('sed -e "s/<om_node>/%s/g" '
                                              '-e "s/<install_node>/%s/g" '
                                              '-e "1,$w %s" %s ' %
                                             (om_node.nd_conf_dict['ip'],
                                              #node.nd_conf_dict['ip'],
                                              inventory_machine_list,
                                              tempInventory.name,
                                              templateDir + install.nd_conf_dict['inventory-template']))

        if status != 0:
            self.log.error("Ansible inventory file generation failed.")
            return False

        self.log.debug("Generated ansible inventory file:\n%s" % tempInventory)
        tempInventory.seek(0)
        for line in tempInventory:
            self.log.debug(line)

        # Probably not needed ...
        tempInventory.seek(0)

        cur_dir = os.getcwd()
        os.chdir(install.nd_conf_dict['deploy-script-dir'])

        # Run the installation. Let's log the stdout contents.
        status, stdout = localhost.nd_agent.exec_wait('./%s %s %s ' %
                                             (install.nd_conf_dict['deploy-script'],
                                              tempInventory.name,
                                              install.nd_conf_dict['deb-location']),
                                              return_stdin=True)
        tempInventory.close()

        os.chdir(cur_dir)

        if status != 0:
            self.log.error("FDS package installation on node %s returned status %d." %
                           (node.nd_conf_dict['node-name'], status))
            return False

        for line in stdout.splitlines():
            self.log.info("[%s] %s" % (node.nd_conf_dict['ip'], line))

        # TODO(GREG): Until Ansible can install without booting up anything,
        # we kill the domain Ansible booted and clean up log files, etc. to give
        # the test a clean installation with which to work.
        domainKill = TestFDSSysMgt.TestNodeKill(parameters=self.parameters)
        testCaseSuccess = domainKill.test_NodeKill(ansibleBoot=True)
        if not testCaseSuccess:
            self.log.error("FDS package installation domain kill failed.")
            return False

        return True


# This class contains attributes and methods to test
# deleting an FDS installation or root directory.
class TestFDSDeleteInstDir(TestCase.FDSTestCase):
    def __init__(self, parameters = None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_FDSDeleteInstDir,
                                             "FDS installation directory deletion")

        self.passedNode = node

    def test_FDSDeleteInstDir(self):
        """
        Test Case:
        Attempt to delete the FDS installation directory structure.
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        # If we have core files, return a failure and don't remove anything.
        cur_dir = os.getcwd()
        os.chdir(fdscfg.rt_env.get_fds_source() + "/..")
        rc = subprocess.call(["bash", "-c", ". ./jenkins_scripts/core_hunter.sh; core_hunter"])
        os.chdir(cur_dir)

        # Note: core_hunter is looking for core files and returns a "success" code
        # when it finds at least one.
        if rc == 0:
            self.log.error("Core files detected.")
            return False
        else:
            self.log.info("No cores found.")

        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            # If we were passed a specific node, use it and get it.
            if self.passedNode is not None:
                n = self.passedNode

            fds_dir = n.nd_conf_dict['fds_root']

            # Check to see if the FDS root directory is already there.
            status = n.nd_agent.exec_wait('ls ' + fds_dir)
            if status == 0:
                # Try to delete it.
                self.log.info("FDS installation directory, %s, exists on node %s. Attempting to delete." %
                              (fds_dir, n.nd_conf_dict['node-name']))

                status = n.nd_agent.exec_wait('rm -rf \"%s\"' % fds_dir)

                if status != 0:
                    self.log.error("FDS installation directory deletion on node %s returned status %d." %
                                   (n.nd_conf_dict['node-name'], status))
                    return False
            else:
                self.log.warn("FDS installation directory, %s, nonexistent on node %s." %
                              (fds_dir, n.nd_conf_dict['node-name']))

            if self.passedNode is not None:
                # We're done with the specified node. Get out.
                break

        return True


# This class contains attributes and methods to test clean shared memory.
# A workaround that is presently required if you want to restart a domain
# that was previously started.
class TestFDSSharedMemoryClean(TestCase.FDSTestCase):
    def __init__(self, parameters = None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_FDSSharedMemoryClean,
                                             "Remove /dev/shm/0x* entries")

        self.passedNode = node

    def test_FDSSharedMemoryClean(self):
        """
        Test Case:
        Attempt to selectively delete from the FDS installation directory.
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]
        bin_dir = fdscfg.rt_env.get_bin_dir(debug=False)

        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            # If we were passed a specific node, use it and get it.
            if self.passedNode is not None:
                n = self.passedNode

            # Delete any shared memory segments found
            status = n.nd_agent.exec_wait('find /dev/shm -name "0x*" -exec rm {} \;')
            if status == 0:
                self.log.info("Shared memory segments deleted on node %s" %
                              (n.nd_conf_dict['node-name']))
            else:
                self.log.warn("Failed to delete shared memory segments on node %s." %
                              (n.nd_conf_dict['node-name']))

            if self.passedNode is not None:
                # We're done with the specified node. Get out.
                break
        return True

# This class contains attributes and methods to test
# clean selective parts of an FDS installation directory.
class TestFDSSelectiveInstDirClean(TestCase.FDSTestCase):
    def __init__(self, parameters = None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_FDSSelectiveInstDirClean,
                                             "Selective FDS installation directory clean")

        self.passedNode = node

    def test_FDSSelectiveInstDirClean(self):
        """
        Test Case:
        Attempt to selectively delete from the FDS installation directory.
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]
        bin_dir = fdscfg.rt_env.get_bin_dir(debug=False)

        # If we have core files, return a failure and don't remove anything.
        cur_dir = os.getcwd()
        os.chdir(fdscfg.rt_env.get_fds_source() + "/..")
        rc = subprocess.call(["bash", "-c", ". ./jenkins_scripts/core_hunter.sh; core_hunter"])
        os.chdir(cur_dir)

        # Note: core_hunter is looking for core files and returns a "success" code
        # when it finds at least one.
        if rc == 0:
            self.log.error("Core files detected.")
            return False
        else:
            self.log.info("No cores found.")

        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            # If we were passed a specific node, use it and get it.
            if self.passedNode is not None:
                n = self.passedNode

            self.log.info("Attempting to selectively clean node %s." % n.nd_conf_dict['node-name'])

            status = n.nd_cleanup_node(test_harness=True, _bin_dir=bin_dir)

            if status != 0:
                self.log.error("FDS installation directory selective clean on node %s returned status %d." %
                               (n.nd_conf_dict['node-name'], status))
                return False

            if self.passedNode is not None:
                # We're done with the specified node. Get out.
                break

        return True


# This class contains the attributes and methods to test
# restarting Redis to a "clean" state.
class TestRestartRedisClean(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_RestartRedisClean,
                                             "Restart Redis clean")

        self.passedNode = node

    def test_RestartRedisClean(self):
        """
        Test Case:
        Attempt to restart Redis to a "clean" state.
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        # If we were passed a node, use it and get out. Otherwise,
        # we use the one captured as the OM node during config parsing.
        if self.passedNode is not None:
            n = self.passedNode
        else:
            n = fdscfg.rt_om_node

        self.log.info("Restart Redis clean on %s." % n.nd_conf_dict['node-name'])

        status = n.nd_agent.exec_wait("./redis.sh restart", fds_tools=True)
        time.sleep(2)

        if status != 0:
            self.log.error("Restart Redis before clean on %s returned status %d." % (n.nd_conf_dict['node-name'], status))
            return False

        status = n.nd_agent.exec_wait("./redis.sh clean", fds_tools=True)
        time.sleep(2)

        if status != 0:
            self.log.error("Clean Redis on %s returned status %d." % (n.nd_conf_dict['node-name'], status))
            return False

        status = n.nd_agent.exec_wait("./redis.sh restart", fds_tools=True)
        time.sleep(2)

        if status != 0:
            self.log.error("Restart Redis after clean on %s returned status %d." % (n.nd_conf_dict['node-name'], status))
            return False

        return True


# This class contains the attributes and methods to test
# booting up Redis.
class TestBootRedis(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_BootRedis,
                                             "Boot Redis")

        self.passedNode = node

    def test_BootRedis(self):
        """
        Test Case:
        Attempt to boot Redis.
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        # If we were passed a node, use it and get out. Otherwise,
        # we use the one captured as the OM node during config parsing.
        if self.passedNode is not None:
            n = self.passedNode
        else:
            n = fdscfg.rt_om_node

        self.log.info("Boot Redis on node %s." %n.nd_conf_dict['node-name'])

        status = n.nd_agent.exec_wait("./redis.sh start", fds_tools=True)
        time.sleep(2)

        if status != 0:
            self.log.error("Boot Redis on node %s returned status %d." % (n.nd_conf_dict['node-name'], status))
            return False

        return True


# This class contains the attributes and methods to test
# shutting down Redis.
class TestShutdownRedis(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_ShutdownRedis,
                                             "Shutdown Redis",
                                             True)  # Always execute.

        self.passedNode = node

    def test_ShutdownRedis(self):
        """
        Test Case:
        Attempt to shutdown Redis.
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        # If we were passed a node, use it and get out. Otherwise,
        # we use the one captured as the OM node during config parsing.
        if self.passedNode is not None:
            n = self.passedNode
        else:
            n = fdscfg.rt_om_node

        self.log.info("Shutdown Redis on node %s." %n.nd_conf_dict['node-name'])

        status = n.nd_agent.exec_wait("./redis.sh stop", fds_tools=True)
        time.sleep(2)

        # Status == 127 on a remote install if redis.sh does not exist.
        if (status != 0) and (n.nd_agent.env_install and status != 127):
            self.log.error("Shutdown Redis on node %s returned status %d." % (n.nd_conf_dict['node-name'], status))
            return False

        return True


# This class contains the attributes and methods to test
# verifying that Redis is up.
class TestVerifyRedisUp(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_VerifyRedisUp,
                                             "Verify Redis is up")

        self.passedNode = node

    def test_VerifyRedisUp(self):
        """
        Test Case:
        Attempt to verify Redis is up.
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        # If we were passed a node, use it and get out. Otherwise,
        # we use the one captured as the OM node during config parsing.
        if self.passedNode is not None:
            n = self.passedNode
        else:
            n = fdscfg.rt_om_node

        self.log.info("Verify Redis is up node %s." % n.nd_conf_dict['node-name'])

        # Parameter return_stdin is set to return stdout. ... Don't ask me!
        status, stdout = n.nd_agent.exec_wait("./redis.sh status", return_stdin=True, fds_tools=True)

        if status != 0:
            self.log.error("Verify Redis is up on node %s returned status %d." % (n.nd_conf_dict['node-name'], status))
            return False

        self.log.info(stdout)

        if stdout.count("NOT") > 0:
            return False

        return True


# This class contains the attributes and methods to test
# verifying that Redis is down.
class TestVerifyRedisDown(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_VerifyRedisDown,
                                             "Verify Redis is down")

        self.passedNode = node

    def test_VerifyRedisDown(self):
        """
        Test Case:
        Attempt to verify Redis is Down.
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        # If we were passed a node, use it and get out. Otherwise,
        # we use the one captured as the OM node during config parsing.
        if self.passedNode is not None:
            n = self.passedNode
        else:
            n = fdscfg.rt_om_node

        self.log.info("Verify Redis is down node %s." % n.nd_conf_dict['node-name'])

        # Parameter return_stdin is set to return stdout. ... Don't ask me!
        status, stdout = n.nd_agent.exec_wait("./redis.sh status", return_stdin=True, fds_tools=True)

        # Status == 127 on a remote install if redis.sh does not exist.
        if (status != 0) and (n.nd_agent.env_install and status != 127):
            self.log.error("Verify Redis is down on node %s returned status %d." % (n.nd_conf_dict['node-name'], status))
            return False

        if len(stdout) > 0:
            self.log.info(stdout)

        if (stdout.count("NOT") == 0) and not (n.nd_agent.env_install and status == 127):
            return False

        return True

# This class contains the attributes and methods to test
# restarting InfluxDB to a "clean" state.
class TestRestartInfluxDBClean(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_RestartInfluxDBClean,
                                             "Restart InfluxDB clean")

        self.passedNode = node

    def test_RestartInfluxDBClean(self):
        """
        Test Case:
        Attempt to restart InfluxDB to a "clean" state.
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        # If we were passed a node, use it and get out. Otherwise,
        # we use the one captured as the OM node during config parsing.
        if self.passedNode is not None:
            n = self.passedNode
        else:
            n = fdscfg.rt_om_node

        self.log.info("Restart InfluxDB clean on %s." % n.nd_conf_dict['node-name'])

        # issue the clean request (drop dbs etc)
        status = n.nd_clean_influxdb()

        # restart influx
        status = n.nd_agent.exec_wait("service influxdb restart")
        time.sleep(2)

        if status != 0:
            self.log.error("Restart InfluxDB after clean on %s returned status %d." % (n.nd_conf_dict['node-name'], status))
            return False

        return True

# This class contains the attributes and methods to test
# booting up InfluxDB.
class TestBootInfluxDB(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_BootInfluxDB,
                                             "Boot InfluxDB")

        self.passedNode = node

    def test_BootInfluxDB(self):
        """
        Test Case:
        Attempt to boot InfluxDB.
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        # If we were passed a node, use it and get out. Otherwise,
        # we use the one captured as the OM node during config parsing.
        if self.passedNode is not None:
            n = self.passedNode
        else:
            n = fdscfg.rt_om_node

        # Make sure we don't try to boot influx if it wasn't actually configured to do so
        boot_influx = n.nd_conf_dict.get('influxdb', False)
        if isinstance(boot_influx, str):
            if boot_influx.lower() == 'false':
                boot_influx = False

        if not boot_influx:
            self.log.warn('InfluxDB was not configured for {} returning True'.format(n.nd_conf_dict['node-name']))
            return True

        self.log.info("Boot InfluxDB on node %s." %n.nd_conf_dict['node-name'])
        status = n.nd_start_influxdb()
        time.sleep(2)

        if status != 0:
            self.log.error("Boot InfluxDB on node %s returned status %d." % (n.nd_conf_dict['node-name'], status))
            return False

        return True


# This class contains the attributes and methods to test
# shutting down InfluxDB.
class TestShutdownInfluxDB(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_ShutdownInfluxDB,
                                             "Shutdown InfluxDB",
                                             True)  # Always execute.

        self.passedNode = node

    def test_ShutdownInfluxDB(self):
        """
        Test Case:
        Attempt to shutdown InfluxDB.
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        # If we were passed a node, use it and get out. Otherwise,
        # we use the one captured as the OM node during config parsing.
        if self.passedNode is not None:
            n = self.passedNode
        else:
            n = fdscfg.rt_om_node

        self.log.info("Shutdown InfluxDB on node %s." %n.nd_conf_dict['node-name'])

        status = n.nd_agent.exec_wait("service influxdb stop")
        time.sleep(2)

        # If we get a 1 from a remote (i.e. "install" or  over Paramiko connection) execution, we're good.
        if (status != 0) and (n.nd_agent.env_install and status != 1):
            self.log.error("Shutdown InfluxDB on node %s returned status %d." % (n.nd_conf_dict['node-name'], status))
            return False

        return True


# This class contains the attributes and methods to test
# verifying that InfluxDB is up.
class TestVerifyInfluxDBUp(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_VerifyInfluxDBUp,
                                             "Verify InfluxDB is up")

        self.passedNode = node

    def test_VerifyInfluxDBUp(self):
        """
        Test Case:
        Attempt to verify InfluxDB is up.
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        # If we were passed a node, use it and get out. Otherwise,
        # we use the one captured as the OM node during config parsing.
        if self.passedNode is not None:
            n = self.passedNode
        else:
            n = fdscfg.rt_om_node

        self.log.info("Verify InfluxDB is up node %s." % n.nd_conf_dict['node-name'])

        # Parameter return_stdin is set to return stdout. ... Don't ask me!
        status, stdout = n.nd_agent.exec_wait("service influxdb status", return_stdin=True)

        if status != 0:
            self.log.error("Verify InfluxDB is up on node %s returned status %d." % (n.nd_conf_dict['node-name'], status))
            return False

        self.log.info(stdout)

        if stdout.count("NOT") > 0:
            return False

        return True


# This class contains the attributes and methods to test
# verifying that InfluxDB is down.
class TestVerifyInfluxDBDown(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_VerifyInfluxDBDown,
                                             "Verify InfluxDB is down")

        self.passedNode = node

    def test_VerifyInfluxDBDown(self):
        """
        Test Case:
        Attempt to verify InfluxDB is Down.
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        # If we were passed a node, use it and get out. Otherwise,
        # we use the one captured as the OM node during config parsing.
        if self.passedNode is not None:
            n = self.passedNode
        else:
            n = fdscfg.rt_om_node

        self.log.info("Verify InfluxDB is down node %s." % n.nd_conf_dict['node-name'])

        # Parameter return_stdin is set to return stdout. ... Don't ask me!
        status = n.nd_agent.exec_wait("service influxdb status 2>&1 >> /dev/null", return_stdin=False)

        # influxdb init script status returns 3 if the process is down. 1 if remote.
        if (status != 3) and (n.nd_agent.env_install and status != 1):
            self.log.error("Verify InfluxDB is down on node %s returned status %d." % (n.nd_conf_dict['node-name'], status))
            return False

        return True


class TestModifyPlatformConf(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None, applyAll=None, **kwargs):
        '''
        Uses sed to modify particular lines in platform.conf. Should be used prior to startup but after install.
        :param parameters: Params filled in by .ini file
        :current*: String in platform.conf to repalce e.g. authentication=true
        :replace*: String to replace current_string with. e.g. authentication=false
        :node: FDS node
        :applyAll: Change the platform.conf file that affects all new and uninstantiated nodes
        '''

        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_TestModifyPlatformConf,
                                             "Modify Platform.conf")

        self.replace = []
        for key, value in kwargs.iteritems():
            if key.startswith('current'):
                replace_key = 'replace' + key[7:]
                replace_value = kwargs.get(replace_key, None)
                if replace_value == None:
                    err = 'no replacement value found for [{}:{}]'.format(key, value)
                    self.log.error(err)
                    raise Exception(err)
                self.replace.append((value, replace_value))
        self.passedNode = node
        self.applyAll = applyAll

  
    def test_TestModifyPlatformConf(self):
        def doit(node):
            if self.parameters['ansible_install_done']:
                plat_file = os.path.join('/fds/etc/platform.conf')
            else:
                if self.applyAll is not None:
                    plat_file = os.path.join('/fds/etc/platform.conf')
                else:
                    plat_file = os.path.join(node.nd_conf_dict['fds_root'], 'etc', 'platform.conf')
            errcode = 0
            for mods in self.replace:
                errcode += node.nd_agent.exec_wait(
                    'sed -ir "s/{}/{}/g" {}'.format(mods[0], mods[1], plat_file))

            return errcode

        fdscfg = self.parameters['fdscfg']
        status = []
        if self.passedNode is not None and self.applyAll is None:
            self.log.info("Modifying platform.conf for node: " + self.passedNode)
            node = findNodeFromInv(fdscfg.rt_obj.cfg_nodes, self.passedNode)
            status.append(doit(node))
        else:
            self.log.info("Modifying platform.conf for all nodes")
            for node in fdscfg.rt_obj.cfg_nodes:
                status.append(doit(node))

        if sum(status) != 0:
            return False
        else:
            return True

if __name__ == '__main__':
    TestCase.FDSTestCase.fdsGetCmdLineConfigs(sys.argv)
    unittest.main()
