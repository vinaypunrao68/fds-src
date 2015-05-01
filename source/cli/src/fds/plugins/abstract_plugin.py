'''
Created on Apr 13, 2015

@author: nate
'''

'''
This is an abstract class to define what methods need
to be overridden by a class that wishes to be a plugin
'''

class AbstractPlugin( object ):

    continuous_protection_str = "continuous_protection"
    priority_str = "priority"
    iops_guarantee_str = "iops_guarantee"
    iops_limit_str = "iops_limit"
    media_policy_str = "media_policy"
    volume_name_str = "volume_name"
    volume_id_str = "volume_id"
    format_str = "format"
    name_str = "name"
    data_str = "data"
    type_str = "type"
    size_str = "size"
    size_unit_str = "size_unit"
    snapshot_id_str = "snapshot_id"
    time_str = "time"
    retention_str = "retention"
    node_id_str = "node_id"
    node_ids_str = "node_ids"
    discovered_str = "discovered"
    added_str = "added"
    all_str = "all"
    services_str = "services"
    state_str = "state"
    domain_name_str = "domain_name"


    def __init__(self, session):
        self.__session = session

    def build_parser(self, parentParser, session): 
        raise NotImplementedError( "Required method for an FDS CLI plugin.")
    
    def detect_shortcut(self, args):
        raise NotImplementedError( "Required method for an FDS CLI plugin.")
    
    @property
    def session(self):
        return self.__session