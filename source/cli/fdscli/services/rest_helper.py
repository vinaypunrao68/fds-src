'''
Created on Apr 3, 2015

@author: nate
'''
import requests
import json
from model.fds_error import FdsError
from tabulate import tabulate
from services.fds_auth_error import FdsAuthError

class RESTHelper():
    
    AUTH_FAILED = "Authentication Failed."
    
    def __init__(self):
        return
    
    def buildHeader(self, session):
        
        t = session.get_token()
        
        return { "FDS-Auth" : t }   
        
    def defaultSuccess(self, response):
    
        print("\n")
        
        if ( len(response) == 0 ):
            return
        else:
            print(tabulate( response, "keys" ))
            print("\n")
    
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
        
        print(err_message)
        
        return err_message
            
    def post(self, session, url, data=None, successCallback=defaultSuccess, failureCallback=defaultErrorHandler):
        
        try:
            header = self.buildHeader(session)
        except FdsAuthError as fae:
            return FdsError( error=fae.error_code, message=fae.message)
        
        response = requests.post( url, data=data, headers=header, verify=False )
        
        if ( response.ok is False ):
            message = failureCallback( self, response )
            return FdsError( error=response, message=message )
        
        rj = response.json()
        return rj
    
    def put(self, session, url, data=None, successCallback=defaultSuccess, failureCallback=defaultErrorHandler):
        
        try:
            header = self.buildHeader(session)
        except FdsAuthError as fae:
            return FdsError( error=fae.error_code, message=fae.message)
                
        response = requests.put( url, data=data, headers=header, verify=False )
        
        if ( response.ok is False ):
            message = failureCallback( self, response )
            return FdsError( error=response, message=message )
        
        rj = response.json()
        return rj    
    
    def get(self, session, url, params={}, successCallback=defaultSuccess, failureCallback=defaultErrorHandler):
        
        try:
            header = self.buildHeader(session)
        except FdsAuthError as fae:
            return FdsError( error=fae.error_code, message=fae.message)      
        
        response = requests.get( url, params=params, headers=header, verify=False )
        
        if ( response.ok is False ):
            message = failureCallback( self, response )
            return FdsError( error=response, message=message )
        
        rj = response.json()
        return rj
    
    def delete(self, session, url, params={}, successCallback=defaultSuccess, failureCallback=defaultErrorHandler):
        
        try:
            header = self.buildHeader(session)
        except FdsAuthError as fae:
            return FdsError( error=fae.error_code, message=fae.message)     
        
        response = requests.delete( url, params=params, headers=header, verify=False)
        
        if ( response.ok is False ):
            message = failureCallback( self, response )
            return FdsError( error=response, message=message )
        
        rj = response.json()
        return rj