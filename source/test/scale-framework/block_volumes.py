import boto
import os
import logging
import random
import shutil
import sys
import json
import requests

import config
import s3
import samples
import users
import utils

class BlockVolumes(object):

    logging.basicConfig(level=logging.INFO)
    log = logging.getLogger(__name__)
    
    def __init__(self, om_ip_address):
        self.om_ip_address = om_ip_address
    
    def create_block_volumes(self, quantity, name, size, unit):
        for i in xrange(0, quantity):
            volume_name = "%s_%s" % (name, i)
            self.__create_block_volume(volume_name, size, unit)

    def __create_block_volume(self, volume_name, size, unit):
        r = None
        port = config.FDS_REST_PORT
        try:
            #Get the user token
            userToken = str(utils.get_user_token("admin", "admin", 
                                                 self.om_ip_address, port, 0, 1))
            self.log.info("userToken = %s", userToken)

            #Setup for the request
            url = "http://" + self.om_ip_address + ":" + `port` + "/api/config/volumes"
            self.log.info("url = %s", url)
            header = {'FDS-Auth' : userToken}
            self.log.info("header = %s", header)
            # prepare data
            data = {"sla":0,"limit":0,"priority":10,"snapshotPolicies":[],
                    "timelinePolicies":[{"retention":604800,
                    "recurrenceRule":{"FREQ":"DAILY"}},{"retention":1209600,
                    "recurrenceRule":{"FREQ":"WEEKLY"}},{"retention":5184000,
                    "recurrenceRule":{"FREQ":"MONTHLY"}},{"retention":31622400,
                    "recurrenceRule":{"FREQ":"YEARLY"}}],"commit_log_retention":86400,
                    "data_connector":{"type":"BLOCK","api":"Basic, Cinder",
                    "options":{"max_size":"100","unit":["GB","TB","PB"]},
                    "attributes":{"size": size,"unit": unit}},"name":volume_name}

            json_data = json.dumps(data)
            #create volume
            r = requests.post(url, data=json_data, headers=header)
            if r is None:
                raise ValueError, "r is None"
            else:
                self.log.info("request = %s", r.request)
                self.log.info("response = %s", r.json())
                self.log.info("status = %s", r.status_code)

                #Check return code
                assert 200 == r.status_code
        except Exception, e:
            self.log.exception(e)

    def delete_block_volume(self, volume_name):
        r = None
        port = config.FDS_REST_PORT
        try:
            #Get the user token
            userToken = str(utils.get_user_token("admin", "admin",
                                                 self.om_ip_address, port, 0, 1))
            self.log.info("userToken = %s", userToken)

            #Setup for the request
            url = "http://" + self.om_ip_address + ":" + `port` + "/api/config/volumes"
            self.log.info("url = %s", url)
            header = {'FDS-Auth' : userToken}
            self.log.info("header = %s", header)
            # prepare data
            data = {"sla":0,"limit":0,"priority":10,"snapshotPolicies":[],
                    "timelinePolicies":[{"retention":604800,
                    "recurrenceRule":{"FREQ":"DAILY"}},{"retention":1209600,
                    "recurrenceRule":{"FREQ":"WEEKLY"}},{"retention":5184000,
                    "recurrenceRule":{"FREQ":"MONTHLY"}},{"retention":31622400,
                    "recurrenceRule":{"FREQ":"YEARLY"}}],"commit_log_retention":86400,
                    "data_connector":{"type":"BLOCK","api":"Basic, Cinder",
                    "options":{"max_size":"100","unit":["GB","TB","PB"]},
                    "attributes":{"size":100,"unit":"GB"}},"name":volume_name}

            json_data = json.dumps(data)
            #create volume
            r = requests.delete(url, data=json_data, headers=header)
            if r is None:
                raise ValueError, "r is None"
            else:
                self.log.info("request = %s", r.request)
                self.log.info("response = %s", r.json())
                self.log.info("status = %s", r.status_code)

                #Check return code
                assert 200 == r.status_code
        except Exception, e:
            self.log.exception(e)
