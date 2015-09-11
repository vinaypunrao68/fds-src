'''
Created on Apr 3, 2015

@author: nate
'''
import tabulate
import json
import time
from collections import OrderedDict

from utils.converters.volume.recurrence_rule_converter import RecurrenceRuleConverter
from model.health.system_health import SystemHealth
from model.health.health_category import HealthCategory

class ResponseWriter():
    
    @staticmethod
    def write_not_implemented(args=None):
        print("\nThis feature is not yet available, but the fine people at Formation Data System are working tirelessly to make this a reality in the near future.\n")
    
    @staticmethod
    def writeTabularData( data, headers="keys" ):
        
        print("\n")
        
        if ( len(data) == 0 ):
            return
        else:
            print(tabulate.tabulate( data, headers=headers ))
            print("\n")

        
    @staticmethod
    def writeJson( data ):
        print("\n" + json.dumps( data, indent=4, sort_keys=True ) + "\n")
        
    @staticmethod
    def prep_volume_for_table( session, response ):
        '''
        making the structure table friendly
        '''        
        
        prepped_responses = []
        
        # Some calls return a singular volume instead of a list, but if we put it in a list we can re-use this algorithm
        if ( isinstance( response, dict ) ):
            response = [response]
        
        for volume in response:
            
            #figure out what to show for last firebreak occurrence
            lastFirebreak = int(volume.status.last_capacity_firebreak)
            lastFirebreakType = "Capacity"
            
            if ( int(volume.status.last_performance_firebreak) > lastFirebreak ):
                lastFirebreak = int(volume.status.last_performance_firebreak)
                lastFirebreakType = "Performance"
                
            if ( lastFirebreak == 0 ):
                lastFirebreak = "Never"
                lastFirebreakType = ""
            else:
                lastFirebreak = time.localtime( lastFirebreak )
                lastFirebreak = time.strftime( "%c", lastFirebreak )
            
            #sanitize the IOPs guarantee value
            iopsMin = volume.qos_policy.iops_min
            
            if ( iopsMin == 0 ):
                iopsMin = "None"
                
            #sanitize the IOPs limit
            iopsLimit = volume.qos_policy.iops_max
            
            if ( iopsLimit == 0 ):
                iopsLimit = "Unlimited"

            ov = OrderedDict()
            
            ov["ID"] = volume.id
            ov["Name"] = volume.name
            
            if ( session.is_allowed( "TENANT_MGMT" ) ):
                if volume.tenant is not None:
                    ov["Tenant"] = volume.tenant.name
                else:
                    ov["Tenant"] = ""
                    
                
            ov["State"] = volume.status.state
            ov["Type"] = volume.settings.type
            ov["Usage"] = str(volume.status.current_usage.size) + " " + volume.status.current_usage.unit
            ov["Last Firebreak Type"] = lastFirebreakType
            ov["Last Firebreak"] = lastFirebreak
            ov["Priority"] = volume.qos_policy.priority
            ov["IOPs Guarantee"] = iopsMin
            ov["IOPs Limit"] = iopsLimit
            ov["Media Policy"] = volume.media_policy
            
            prepped_responses.append( ov )
        #end of for loop 
            
        return prepped_responses
    
    @staticmethod
    def prep_snapshot_for_table( session, response ):
        '''
        Take the snapshot response and make it consumable for a table
        '''
        
        #The tabular format is very poor for a volume object, so we need to remove some keys before display
        resultList = []
        
        for snapshot in response:
            
            created = time.localtime( snapshot.created )
            created = time.strftime( "%c", created )
            
            retentionValue = snapshot.retention
            
            if ( retentionValue == 0 ):
                retentionValue = "Forever"
            
            ov = OrderedDict()
            
            ov["ID"] = snapshot.id
            ov["Name"] = snapshot.name
            ov["Created"] = created
            ov["Retention"] = retentionValue
            
            resultList.append( ov )   
        #end for loop
        
        return resultList
    
    @staticmethod
    def prep_snapshot_policy_for_table( session, response ):
        ''' 
        Take a snapshot policy and format it for easy display in a table
        '''
        
        results = []
        
        for policy in response:
            
            retentionValue = policy.retention_time_in_seconds
            
            if ( retentionValue == 0 ):
                retentionValue = "Forever"
        
            ov = OrderedDict()
            
            if policy.id != None:
                ov["ID"] = policy.id
            
            if ( policy.name != None and policy.name != "" ):
                ov["Name"] = policy.name
                
            ov["Retention"] = retentionValue
            ov["Recurrence Rule"] = RecurrenceRuleConverter.to_json( policy.recurrence_rule )
            
            results.append( ov )
        # end of for loop
        
        return results
            
    
    @staticmethod
    def prep_domains_for_table( session, response ):
        '''
        Take the domain JSON and turn it into a table worthy presentation
        '''
        results = []
        
        for domain in response:
            
            ov = OrderedDict()
            
            ov["ID"] = domain.id
            ov["Name"] = domain.name
            ov["Site"] = domain.site
            
            results.append( ov )
        #end of for loop
        
        return results
    
    
    @staticmethod
    def prep_node_for_table( session, response ):
        '''
        Take nodes and format them to be listed in a table
        '''
        
        results = []
        
        for node in response:
            
            ov = OrderedDict()
            
            ov["ID"] = node.id
            ov["Name"] = node.name
            ov["State"] = node.state
            ov["IP V4 Address"] = node.address.ipv4address
            
            results.append( ov )
        
        return results
    
    @staticmethod
    def prep_services_for_table( session, response ):
        '''
        The service model is not yet sensible so we need to do some munging to get anything
        useful to the Screen.
        '''
        results = []
        
        for node in response:
            
            # we'll need this data for each service
            
            services = node.services
            
            for service_type in services:
                
                for service in services[service_type]:
                    
                    ov = OrderedDict()
                    ov["Node ID"] = node.id
                    ov["Node Name"] = node.name
                    ov["Service Type"] = service.name
                    ov["Service ID"] = service.id
                    ov["State"] = service.status.state
                    
                    results.append( ov )
                    
                # end of individual service for loop
            #end of service_typess for loop
            
        #end of nodes for loop
        
        return results
        
    @staticmethod
    def prep_qos_presets( presets):
        '''
        Prep a list of qos presets for tabular printing
        '''
        
        results = []
        
        for preset in presets:
            
            ov = OrderedDict()
            
            iops_guarantee = preset.iops_guarantee
            
            if iops_guarantee == 0:
                iops_guarantee = "None"
                
            iops_limit = preset.iops_limit
            
            if iops_limit == 0:
                iops_limit = "Forever"
            
            ov["ID"] = preset.id
            ov["Name"] = preset.name
            ov["Priority"] = preset.priority
            ov["IOPs Guarantee"] = iops_guarantee
            ov["IOPs Limit"] = iops_limit
            
            results.append( ov )
            
        return results
            
    @staticmethod
    def prep_users_for_table(users):
        '''
        Put the list of users in a nice readable tabular format
        '''
        
        d_users = []
        
        for user in users:
            ov = OrderedDict()
            
            ov["ID"] = user.id
            ov["Username"] = user.name
            
            d_users.append( ov )
            
        return d_users
        
    @staticmethod
    def prep_tenants_for_table(tenants):
        '''
        Scrub the tenant objects for a readable tabular format
        '''
        
        d_tenants = []
        
        for tenant in tenants:
            ov = OrderedDict()
            
            ov["ID"] = tenant.id
            ov["Name"] = tenant.name
            d_tenants.append( ov )
            
        return d_tenants
    
    @staticmethod
    def prep_system_health_for_table(health):
        
#         if not isinstance(health, SystemHealth):
#             raise TypeError()
        
        ov = OrderedDict()
        
        cap_record = None
        service_record = None
        fb_record = None
        
        for record in health.health_records:
            if record.category == HealthCategory.CAPACITY:
                cap_record = record
            elif record.category == HealthCategory.SERVICES:
                service_record = record
            elif record.category == HealthCategory.FIREBREAK:
                fb_record = record
                
        
        ov["Overall"] = health.overall_health
        ov["Capacity"] = cap_record.state
        ov["Services"] = service_record.state
        ov["Firebreak"] = fb_record.state
        
        results =[ov]
        
        return results
        
