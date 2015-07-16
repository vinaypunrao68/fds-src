
class DataProtectionPolicy(object):
    '''
    Created on May 29, 2015
    
    @author: nate
    '''
    
    def __init__(self, commit_log_retention=86400, preset_id=-1):
        self.commit_log_retention = commit_log_retention
        self.preset_id = preset_id
        self.snapshot_policies = []
        
    @property
    def commit_log_retention(self):
        return self.__commit_log_retention
    
    @commit_log_retention.setter
    def commit_log_retention(self, retention):
        
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
        
        self.__preset_id = preset_id
    
