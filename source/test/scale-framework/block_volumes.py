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
import lib

class BlockVolumes(object):

    logging.basicConfig(level=logging.INFO)
    log = logging.getLogger(__name__)

    def __init__(self, om_ip_address):
        self.om_ip_address = om_ip_address
        self.volume_list = []

    def create_volumes(self, quantity_int, volume_prefix_str, size_str, unit_str, userToken_str=None):
        if userToken_str == None:
            # Get the user token
            userToken_str = str(lib.get_user_token("admin", "admin",
                self.om_ip_address, config.FDS_REST_PORT, 0, 1))

        # Create the volumes
        for i in xrange(0, quantity_int):
            volume_name_str = "%s_%s" % (volume_prefix_str, i)
            self.__create_volume(volume_name_str, size_str, unit_str, userToken_str)

        return self.volume_list

    def __create_volume(self, volume_name_str, size_str, unit_str, userToken_str):
        request_response = None
        port = config.FDS_REST_PORT
        try:
            #Setup for the request
            url_str = "http://" + self.om_ip_address + ":" + `port` + "/api/config/volumes"
            #self.log.info("url = %s", url_str)
            header = {'FDS-Auth' : userToken_str}
            #self.log.info("header = %s", header)
            # prepare data
            data = {"sla":0,"limit":0,"priority":10,"snapshotPolicies":[],
                    "timelinePolicies":[{"retention":604800,
                    "recurrenceRule":{"FREQ":"DAILY"}},{"retention":1209600,
                    "recurrenceRule":{"FREQ":"WEEKLY"}},{"retention":5184000,
                    "recurrenceRule":{"FREQ":"MONTHLY"}},{"retention":31622400,
                    "recurrenceRule":{"FREQ":"YEARLY"}}],"commit_log_retention":86400,
                    "data_connector":{"type":"BLOCK","api":"Basic, Cinder",
                    "options":{"max_size":"100","unit":["GB","TB","PB"]},
                    "attributes":{"size": size_str,"unit": unit_str}},"name":volume_name_str}

            json_data = json.dumps(data)

            #create volume
            request_response = requests.post(url_str, data=json_data, headers=header)
            if request_response is None:
                raise ValueError("There was no response from the server.")
            elif 200 == request_response.status_code:
                    self.volume_list.append(volume_name_str)
            else:
                raise ValueError("Failed to create block volume %s.", volume_name_str)

        except Exception, e:
            self.log.exception(e)

    def delete_volumes(self, volume_list):
        for volume in volume_list:
            self.delete_volume(volume)

    def delete_volume(self, volume_name):
        r = None
        port = config.FDS_REST_PORT
        try:
            #Get the user token
            userToken = str(lib.get_user_token("admin", "admin",
                                                 self.om_ip_address, port, 0, 1))
            self.log.info("userToken = %s", userToken)

            #Setup for the request
            url = "http://" + self.om_ip_address + ":" + `port` + "/api/config/volumes/" + volume_name
            self.log.info("url = %s", url)
            header = {'FDS-Auth' : userToken}
            self.log.info("header = %s", header)
            #create volume
            r = requests.delete(url, headers=header)
            if r is None:
                raise ValueError, "r is None"
                return False

            else:
                self.log.info("request = %s", r.request)
                self.log.info("response = %s", r.json())
                self.log.info("status = %s", r.status_code)

                #Check return code
                assert 200 == r.status_code
                return True

        except Exception, e:
            self.log.exception(e)
