#!/usr/bin/env python

# Copyright 2014 by Formation Data Systems, Inc.
#


class FdsService:
    def __init__(self, service_cfg):
        """
        TODO(Rao): currently serive_cfg is same as node_cfg (node config)
        """
        self.service_cfg = service_cfg

    def monitor_log_file(self, reg_ex, wait_time):
        pass


class SMService(FdsService):
    def __init__(self, service_cfg):
        # TODO(Rao): figure out why below gives error
        # super(SMService, self).__init__(service_cfg)
        pass

    def wait_for_healthy_state(self, wait_time_sec=None):
        """
        Waits until sm service is fully healthy.
        For now we will monitor the log file.  Ideally we will thrift endpoint
        to do the monitoring.
        """
        pass

class DMService(FdsService):
    def __init__(self, service_cfg):
        # TODO(Rao): figure out why below gives error
        # super(DMService, self).__init__(service_cfg)
        pass

class AMService(FdsService):
    def __init__(self, service_cfg):
        super(AMService, self).__init__(service_cfg)

class OMService(FdsService):
    def __init__(self, service_cfg):
        super(OMService, self).__init__(service_cfg)
