'''
Created on Apr 3, 2015

@author: nate
'''
import tabulate
import json
import time
from collections import OrderedDict
from utils.recurrence_rule_converter import RecurrenceRuleConverter

class ResponseWriter():
    
    @staticmethod
    def write_not_implemented(args=None):
        print "\nThis feature is not yet available, but the fine people at Formation Data System are working tirelessly to make this a reality in the near future.\n"
    
    @staticmethod
    def writeTabularData( data, headers="keys" ):
        
        print "\n"
        
        if ( len(data) == 0 ):
            return
        else:
            print tabulate.tabulate( data, headers=headers )
            print "\n"

        
    @staticmethod
    def writeJson( data ):
        print "\n" + json.dumps( data, indent=4, sort_keys=True ) + "\n"
        
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
            lastFirebreak = volume.last_capacity_firebreak
            lastFirebreakType = "Capacity"
            
            if ( volume.last_performance_firebreak > lastFirebreak ):
                lastFirebreak = volume.last_performance_firebreak
                lastFirebreakType = "Performance"
                
            if ( lastFirebreak == 0 ):
                lastFirebreak = "Never"
                lastFirebreakType = ""
            else:
                lastFirebreak = time.localtime( lastFirebreak )
                lastFirebreak = time.strftime( "%c", lastFirebreak )
            
            #sanitize the IOPs guarantee value
            iopsMin = volume.iops_guarantee
            
            if ( iopsMin == 0 ):
                iopsMin = "None"
                
            #sanitize the IOPs limit
            iopsLimit = volume.iops_limit
            
            if ( iopsLimit == 0 ):
                iopsLimit = "Unlimited"

            ov = OrderedDict()
            
            ov["ID"] = volume.id
            ov["Name"] = volume.name
            
            if ( session.is_allowed( "TENANT_MGMT" ) ):
                ov["Tenant ID"] = volume.tenant_id
                
            ov["State"] = volume.state
            ov["Type"] = volume.type
            ov["Usage"] = str(volume.current_size) + " " + volume.current_units
            ov["Last Firebreak Type"] = lastFirebreakType
            ov["Last Firebreak"] = lastFirebreak
            ov["Priority"] = volume.priority
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
            
            retentionValue = policy.retention
            
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
            ov["IP V4 Address"] = node.ip_v4_address
            ov["IP V6 Address"] = node.ip_v6_address
            
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
                    ov["Service Type"] = service.auto_name
                    ov["Service ID"] = service.id
                    ov["Status"] = service.status
                    
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
            ov["Username"] = user.username
            
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
        
