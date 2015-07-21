from model.base_model import BaseModel

class DataProtectionPolicyPreset(BaseModel):
    '''
    Created on May 6, 2015
    
    @author: nate
    '''
    
    def __init__(self, an_id=-1, name=None, commit_log_retention=86400, policies=list()):
        BaseModel.__init__(self, an_id, name)
        self.__commit_log_retention = commit_log_retention
        self.__snapshot_policies = policies
        
    @property
    def commit_log_retention(self):
        return self.__commit_log_retention
    
    @commit_log_retention.setter
    def commit_log_retention(self, a_time):
        self.__commit_log_retention = a_time
        
    @property
    def snapshot_policies(self):
        return self.__snapshot_policies
    
    @snapshot_policies.setter
    def policies(self, policies):
        self.__snapshot_policies = policies
