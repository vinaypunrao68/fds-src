from collections import defaultdict
import TestCase
from fabric.api import local
import time
from fdslib.TestUtils import execute_command_with_fabric

debug_tool_path_local = '../../Build/linux-x86_64.debug/tools/fds.debug'
debug_tool_path_remote = '/fds/sbin/fds.debug'


class TestRunGarbageCollector(TestCase.FDSTestCase):
    def __init__(self, parameters=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_RunGC,
                                             "Run garbage collector and wait untill it's done")

    def test_RunGC(self):
        om_ip = self.parameters["fdscfg"].rt_om_node.nd_conf_dict['ip']
        assert self.gc_not_running() is True
        pre_gc_dms_refscan = self.get_dm_service_counters()

        op = run_fds_debug("gc start",self.parameters["ansible_install_done"], om_ip)

        self.log.info("Sleep for a minute. Remove this sleep once gc info view is changed and we do get # of previous gc runs")
        # TODO pooja: remove this sleep once we have count of gc runs in gc info
        time.sleep(60)

        retryCount = 0
        maxRetries = 20
        while retryCount < maxRetries:
            retryCount += 1
            if self.gc_not_running() is False:
                self.log.info("GC is still running, retry after 30 sec sleep")
                time.sleep(30)
                continue
            else:
                assert self.verify_counters_timestamp(pre_gc_dms_refscan) is True
                self.log.info("GC finished. Pre GC dm service counter last run timestapms are greater than post GC")
                return True
        return False

    # This method returns a. True if GC is not running b.False if GC is running
    # Parses output or fds.debug tool's "gc info" command
    def gc_not_running(self):
        flag = False
        om_ip = self.parameters["fdscfg"].rt_om_node.nd_conf_dict['ip']
        op = run_fds_debug("gc info", self.parameters['ansible_install_done'], om_ip)
        dict_a = parse_gc_output(op)

        # To confirm gc is not running, all each item in check_keys list should be zero
        check_keys = {'dm.doing.refscan', 'sm.doing.compaction', 'sm.doing.gc'}
        testval = ['0']
        flag = all(dict_a[key] == testval for key in check_keys)
        return flag

    # @param pre_gc_dms_refscan = list of timestamps for pre gc run
    # If pre_gc_dms_refscan timestamps are greater than respective post_gc_dms_refscan then retrun True else False
    def verify_counters_timestamp(self, pre_gc_dms_refscan):
        post_gc_run = self.get_dm_service_counters()

        # When gc runs first time, pre_gc_dms_ref_scan is empty list
        if len(pre_gc_dms_refscan) != 0:
            assert len(pre_gc_dms_refscan) == len(post_gc_run)
            for idx, value in enumerate(pre_gc_dms_refscan):
                if not int(post_gc_run[idx]) > int(pre_gc_dms_refscan[idx]):
                    self.log.error("Failed: dm.refscan timestamp {0} not greater than previous run {1}".
                              format(post_gc_run[idx], pre_gc_dms_refscan[idx]))
                    self.log.info("dm.refscan timestamps. previous run:{}, Current run {}".
                             format(pre_gc_dms_refscan, post_gc_run))
                    return False
        else:
            self.log.info("This is First GC run so skip dm.refscan.lastrun.timestamp verification")
        return True

    # This method parses output of fds.debug tool's `service counters dm` command
    # returns dictionary as key value pair
    def get_dm_service_counters(self):
        om_ip = self.parameters["fdscfg"].rt_om_node.nd_conf_dict['ip']
        op = run_fds_debug("service counters dm", self.parameters['ansible_install_done'], om_ip)
        counters_dict = parse_counters_output(op)
        return counters_dict['dm.refscan.lastrun.timestamp']


# This class contains the attributes and methods to test
# if SM deleted objects after GC ran
#
# If @param expect_failure is false then verify SM DID delete objects
#    @param expect_failure is true then Verify SM DID NOT delete any objects
# If no paramaeter is passed then expect_failure has default value false
class TestVerifySmDeletion(TestCase.FDSTestCase):
    def __init__(self, parameters=None, expect_failure="false"):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_VerifySmDeletion,
                                             "Check if SM deleted data")
        if expect_failure == "true":
            self.passedExpect_failure = True
        else:
            self.passedExpect_failure = False

    def test_VerifySmDeletion(self):
        """
        Test Case:
        Check if GC deleted data from SM
        """
        om_ip = self.parameters["fdscfg"].rt_om_node.nd_conf_dict['ip']
        op = run_fds_debug("gc info", self.parameters['ansible_install_done'], om_ip)
        key_val_dict = parse_gc_output(op)
        objects_deleted = int(key_val_dict['sm.objects.deleted'][0])

        if self.passedExpect_failure is True:
            # As a part of garbage collection we expect SM side deletion did NOT happen,
            # so tokens deleted should be EQUAL TO Zero, if not mark as fail
            if objects_deleted != 0:
                self.log.error(
                    "Failed: GC deleted {} SM objects".format(objects_deleted))
                return False
        else:
            # As a part of garbage collection we expect SM side deletion DID happen,
            # so objects deleted should be GREATER than 0, if not mark as failure
            if not objects_deleted > 0:
                self.log.error("Failed: GC deleted {} SM objects".format(objects_deleted))
                return False

        self.log.info("OK: GC deleted {} SM objects".format(objects_deleted))
        return True


# This method parses output of `gc info` or `gc start`.
# Note: Duplicate keys are grouped together.
def parse_gc_output(op):
    op = op.split("\n")
    op = [item for item in op if not (item.startswith("--") or item.startswith("=="))]
    op = [item for item in op if not (item.startswith("--") or item.startswith("=="))]
    op_dict = defaultdict(list)
    for each in op:
        a = each.split(" ")
        while '' in a:
            a.remove('')
        if len(a) == 2:
            op_dict[a[0]].append(a[1].rstrip())
    return op_dict


# This method parses output of `service counters dm/sm` .
# Note: Duplicate keys are grouped together.
def parse_counters_output(op):
    op = op.split("\n")
    op = [item for item in op if not (item.startswith("--") or item.startswith("=="))]
    op = [item for item in op if not (item.startswith("--") or item.startswith("=="))]
    op_dict = defaultdict(list)
    for each in op:
        a = each.split(" ")
        while '' in a:
            a.remove('')
        if len(a) == 2:
            op_dict[a[1]].append(a[0])
    return op_dict


# fds.debug is a tool and supports many functions which are not supported by fdscli
# (as need of these functions is for developers not customers) such as smchk. gc, counters info etc
def run_fds_debug(cmd, remote_env, om_ip):
    """
    Arguments
    ----------
    cmd : str : command to run using fds.debug tool
    remote_env: bool: If true then run fds.debug tool on remote node else locally
    om_ip: str : IP address of OM

    Returns:
    --------
    op : Output of command using fds.debug too
    """
    if remote_env:
        cmd = debug_tool_path_remote + " "+cmd
    else:
        cmd = debug_tool_path_local + " "+cmd
    op = execute_command_with_fabric(cmd, remote_env=remote_env, node_ip=om_ip)
    return op
