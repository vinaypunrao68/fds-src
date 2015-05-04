'''
Created on Apr 3, 2015

@author: nate
'''
import requests
import json
import response_writer

class RESTHelper():
    
    def __init__(self):
        return
    
    def buildHeader(self, session):
        return { "FDS-Auth" : session.get_token() }   
        
    def defaultSuccess(self, response):
        response_writer.ResponseWriter.writeTabularData( response )
    
    def defaultErrorHandler(self, error):
        errorText = json.loads( error.text )
        print str(error.status_code) + ": " + errorText["message"]
        return
            
    def post(self, session, url, data=None, successCallback=defaultSuccess, failureCallback=defaultErrorHandler):
        response = requests.post( url, data=data, headers=self.buildHeader( session ), verify=False )
        
        if ( response.ok is False ):
            failureCallback( self, response )
            return
        
        rj = response.json()
        return rj
    
    def put(self, session, url, data=None, successCallback=defaultSuccess, failureCallback=defaultErrorHandler):
        response = requests.put( url, data=data, headers=self.buildHeader( session ), verify=False )
        
        if ( response.ok is False ):
            failureCallback( self, response )
            return
        
        rj = response.json()
        return rj    
    
    def get(self, session, url, params={}, successCallback=defaultSuccess, failureCallback=defaultErrorHandler):
        
        response = requests.get( url, params=params, headers=self.buildHeader( session ), verify=False )
        
        if ( response.ok is False ):
            failureCallback( self, response )
            return
        
        rj = response.json()
        return rj
    
    def delete(self, session, url, params={}, successCallback=defaultSuccess, failureCallback=defaultErrorHandler):
        
        response = requests.delete( url, params=params, headers=self.buildHeader(session), verify=False)
        
        if ( response.ok is False ):
            failureCallback( self, response )
            return
        
        rj = response.json()
        return rj