'''
Created on Apr 3, 2015

@author: nate
'''
import tabulate
import json
import sys

class ResponseWriter():
    
    @staticmethod
    def writeTabularData( data, headers="keys" ):
        
        print tabulate.tabulate( data, headers=headers )
        
    @staticmethod
    def writeJson( data ):
        print json.dumps( data, indent=4, sort_keys=True )