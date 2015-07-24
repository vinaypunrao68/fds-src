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
    iops_guarantee_str = "iops_min"
    iops_limit_str = "iops_max"
    media_policy_str = "media_policy"
    volume_name_str = "volume_name"
    volume_id_str = "volume_id"
    volume_ids_str = "volume_ids"
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
    service_str = "service"
    service_id_str = "service_id"
    state_str = "state"
    domain_name_str = "domain_name"
    domain_id_str = "domain_id"
    recurrence_rule_str = "recurrence_rule"
    frequency_str = "frequency"
    day_of_month_str = "day_of_month"
    day_of_week_str = "day_of_week"
    day_of_year_str = "day_of_year"
    week_str = "week"
    month_str = "month"
    hour_str = "hour"
    minute_str = "minute"
    policy_id_str = "policy_id"    
    timeline_preset_str = "data_protection_preset_id"
    qos_preset_str = "qos_preset_id"
    user_name_str = "username"
    user_id_str = "user_id"
    tenant_id_str = "tenant_id"
    block_size_str= "block_size"
    block_size_unit_str = "block_size_unit"
    max_obj_size_str = "max_object_size"
    max_obj_size_unit_str = "max_object_size_unit"
    metrics_str = "metrics"
    start_str = "start"
    end_str = "end"
    points_str = "points"
    most_recent_str = "most_recent"
    size_for_value_str = "size_for_value"

    def __init__(self):
        '''
        constructor
        '''
        self.__session = None

    def build_parser(self, parentParser, session): 
        raise NotImplementedError( "Required method for an FDS CLI plugin.")
    
    def detect_shortcut(self, args):
        raise NotImplementedError( "Required method for an FDS CLI plugin.")
    
    @property
    def arg_str(self):
        return "-"
    
    @property
    def session(self):
        return self.__session
    
    @session.setter
    def session(self, session):
        self.__session = session
    
    def add_format_arg(self, parser):
        '''
        Add the format argument to the passed in parser
        '''
        parser.add_argument( "-" + AbstractPlugin.format_str, help="Specify the format that the result is printed as", choices=["json","tabular"], required=False )
        