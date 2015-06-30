'''
Created on Apr 3, 2015

@author: nate
'''
import requests
import json
import response_writer
from model.fds_error import FdsError

class RESTHelper():
    
    def __init__(self):
        return
    
    def buildHeader(self, session):
        return { "FDS-Auth" : session.get_token() }   
        
    def defaultSuccess(self, response):
        response_writer.ResponseWriter.writeTabularData( response )
    
    def defaultErrorHandler(self, error):
        
        errorText = "";
        err_message = str(error.status_code) + ": "
        
        try:
            errorText = json.loads( error.text )
        except ValueError:
            #do nothing and go on
            pass
        
        if "message" in errorText:
            err_message += errorText["message"]
        else:
            err_message += error.reason
        
        if error.status_code == 404:
            err_message = "The operation received a 404 response, this means there is a problem communicating with the FDS cluster."
        
        print err_message
        
        return err_message
            
    def post(self, session, url, data=None, successCallback=defaultSuccess, failureCallback=defaultErrorHandler):
        response = requests.post( url, data=data, headers=self.buildHeader( session ), verify=False )
        
        if ( response.ok is False ):
            failureCallback( self, response )
            return FdsError( error=response )
        
        rj = response.json()
        return rj
    
    def put(self, session, url, data=None, successCallback=defaultSuccess, failureCallback=defaultErrorHandler):
        response = requests.put( url, data=data, headers=self.buildHeader( session ), verify=False )
        
        if ( response.ok is False ):
            failureCallback( self, response )
            return FdsError( error=response )
        
        rj = response.json()
        return rj    
    
    def get(self, session, url, params={}, successCallback=defaultSuccess, failureCallback=defaultErrorHandler):
        
        response = requests.get( url, params=params, headers=self.buildHeader( session ), verify=False )
        
        if ( response.ok is False ):
            failureCallback( self, response )
            return FdsError( error=response )
        
        rj = response.json()
        return rj
    
    def delete(self, session, url, params={}, successCallback=defaultSuccess, failureCallback=defaultErrorHandler):
        
        response = requests.delete( url, params=params, headers=self.buildHeader(session), verify=False)
        
        if ( response.ok is False ):
            failureCallback( self, response )
            return FdsError( error=response )
        
        rj = response.json()
        return rj