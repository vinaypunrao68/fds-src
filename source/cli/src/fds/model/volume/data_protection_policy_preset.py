from model.fds_id import FdsId

class DataProtectionPolicyPreset(object):
    '''
    Created on May 6, 2015
    
    @author: nate
    '''
    
    def __init__(self, an_id=FdsId(), commit_log_retention=86400, policies=list()):
        self.id = an_id
        self.__commit_log_retention = commit_log_retention
        self.__snapshot_policies = policies
        
    @property
    def id(self):
        return self.__id
    
    @id.setter
    def id(self, an_id):
        
        if not isinstance(an_id, FdsId):
            raise TypeError()
        
        self.__id = an_id
        
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