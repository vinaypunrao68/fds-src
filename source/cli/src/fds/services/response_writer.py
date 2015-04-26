'''
Created on Apr 3, 2015

@author: nate
'''
import tabulate
import json

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