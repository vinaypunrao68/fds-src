'''
Created on Apr 3, 2015

@author: nate
'''
import tabulate
import json
import time
from collections import OrderedDict
from fds.utils.volume_converter import VolumeConverter 
from fds.utils.snapshot_converter import SnapshotConverter

class ResponseWriter():
    
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
        Flatten the volume JSON object dictionary so it's more user friendly - mainly
        for use with the tabular print option
        '''        
        
        prepped_responses = []
        
        # Some calls return a singular volume instead of a list, but if we put it in a list we can re-use this algorithm
        if ( isinstance( response, dict ) ):
            response = [response]
        
        for jVolume in response:
            volume = VolumeConverter.build_volume_from_json( jVolume )
            
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
                iopsMin = "Unlimited"
                
            #sanitize the IOPs limit
            iopsLimit = volume.iops_limit
            
            if ( iopsLimit == 0 ):
                iopsLimit = "None"
            
            fields = [];
            
            fields.append(("ID", volume.id))
            fields.append(("Name", volume.name))
            
            if ( session.is_allowed( "TENANT_MGMT" ) ):
                fields.append(("Tenant ID", volume.tenant_id))
            
            fields.append(("State", volume.state))
            fields.append(("Type", volume.type))
            fields.append(("Usage", str(volume.current_size) + " " + volume.current_units))
            fields.append(("Last Firebreak Type", lastFirebreakType))
            fields.append(("Last Firebreak", lastFirebreak))
            fields.append(("Priority", volume.priority))
            fields.append(("IOPs Guarantee", iopsMin))
            fields.append(("IOPs Limit", iopsLimit))
            fields.append(("Media Policy", volume.media_policy))
            
            ov = OrderedDict( fields )
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
        
        for snap in response:
            fields = []
            
            snapshot = SnapshotConverter.build_snapshot_from_json( snap )
            created = time.localtime( snapshot.created )
            created = time.strftime( "%c", created )
            
            fields.append( ("ID", snapshot.id))
            fields.append( ("Name", snapshot.name))
            fields.append( ("Created", created))
            
            retentionValue = snapshot.retention
            
            if ( retentionValue == 0 ):
                retentionValue = "Forever"
            
            fields.append( ("Retention", retentionValue))
            
            ov = OrderedDict( fields )
            resultList.append( ov )   
        #end for loop
        
        return resultList