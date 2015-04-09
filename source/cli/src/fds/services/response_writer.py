'''
Created on Apr 3, 2015

@author: nate
'''
import tabulate
import json

class ResponseWriter():
    
    @staticmethod
    def writeTabularData( data ):
        print tabulate.tabulate( data, headers="keys" )
        
    @staticmethod
    def writeJson( data ):
        print json.dumps( data, indent=4, sort_keys=True )