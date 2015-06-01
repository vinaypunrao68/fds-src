from model.common.duration import Duration
from model.fds_id import FdsId

class DataProtectionPolicy(object):
    '''
    Created on May 29, 2015
    
    @author: nate
    '''
    
    def __init__(self, commit_log_retention=Duration(), snapshot_policies=[], preset_id=FdsId()):
        self.commit_log_retention = commit_log_retention
        self.snapshot_policies = snapshot_policies
        self.preset_id = preset_id
        
    @property
    def commit_log_retention(self):
        return self.__commit_log_retention
    
    @commit_log_retention.setter
    def commit_log_retention(self, retention):
        
        if not isinstance(retention, Duration):
            raise TypeError()
        
        self.__commit_log_retention = retention
        
    @property
    def snapshot_policies(self):
        return self.__snapshot_policies
    
    @snapshot_policies.setter
    def snapshot_policies(self, policies):
        
        self.__snapshot_policies = policies
        
    @property
    def preset_id(self):
        return self.__preset_id
    
    @preset_id.setter
    def preset_id(self, preset_id):
        
        if preset_id is not None and not isinstance(preset_id, FdsId):
            raise TypeError()
        
        self.__preset_id = preset_id
    
