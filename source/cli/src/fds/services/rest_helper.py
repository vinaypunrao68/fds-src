'''
Created on Apr 3, 2015

@author: nate
'''
import requests
import response_writer

class RESTHelper():
    
    def __init__(self, token):
        self.__token = token
        return
    
    def getHeaders(self):
        return { "FDS-Auth" : self.__token }   
        
    def defaultSuccess(self, response):
        response_writer.ResponseWriter.writeTabularData( response )
    
    def defaultErrorHandler(self, error):
        return ''
        
#     def login(self, username, password, hostname, port, successCallback, failureCallback=defaultErrorHandler ):
#         payload = { "login" : username, "password" : password }
#         response = requests.post( 'http://' + hostname + ':' + str(port) + '/api/auth/token', params=payload )
#         response = response.json()
#         self.__token = response['token']
#         successCallback()
            
    def post(self, url, data, successCallback=defaultSuccess, failureCallback=defaultErrorHandler):
        return ''
    
    def get(self, url, params={}, successCallback=defaultSuccess, failureCallback=defaultErrorHandler):
        
        response = requests.get( url, params=params, headers=self.getHeaders() )
        rj = response.json()
        return rj