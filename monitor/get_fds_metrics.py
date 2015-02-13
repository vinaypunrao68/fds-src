#!/usr/bin/python

import argparse
import httplib
import influxdb
import json
import logging
import re
import requests
import sys
import time

argparser = argparse.ArgumentParser()

argparser.add_argument('-d', '--debug', help="Set debug level - the higher the level, the further down the rabbit hole...")
argparser.add_argument('-f', '--config', help="Config file to load")

argparser.parse_args()
args = argparser.parse_args()

def verbose_requests():
    httplib.HTTPConnection.debuglevel = 1
    logging.basicConfig()
    logging.getLogger().setLevel(logging.DEBUG)
    requests_log = logging.getLogger("requests.packages.urllib3")
    requests_log.setLevel(logging.DEBUG)
    requests_log.propagate = True

if not args.config:
    print "Could not load module 'telepathy'.\nPlease specify a configuration file via -f"
    exit(1)

def D(level, msg):
    if args.debug and int(args.debug) >= level:
        print "DEBUG{0} :: {1}".format(level, msg)

def INFO(msg):
    print "INFO :: {0}".format(msg)

def WARN(msg):
    print "WARN :: {0}".format(msg)

def ERR(msg):
    print "ERROR :: {0}".format(msg)

def ERREXIT(msg):
    print "ERROR :: {0}".format(msg)
    exit(1)

def parse_config(config_filename):
    # hat tip to riquetd for code to parse comments out of json config files
    comment_re = re.compile(
        '(^)?[^\S\n]*/(?:\*(.*?)\*/[^\S\n]*|/[^\n]*)($)?',
        re.DOTALL | re.MULTILINE
    )

    try:
        with open(config_filename) as f:
            content = ''.join(f.readlines())
            match = comment_re.search(content)
            while match:
                content = content[:match.start()] + content[match.end():]
                match = comment_re.search(content)
            config_data = json.loads(content)
            f.close()

    except ValueError as e:
        ERR ("Failed to load config, invalid JSON:")
        ERREXIT ("  {0}".format(e))

    for val in ['targets', 'db']:
        if not val in config_data:
            ERREXIT ("missing '{0}' config value - exiting.".format(val))

    for val in ['host', 'port', 'name', 'user', 'pass']:
        if not val in config_data['db']:
            ERREXIT ("missing db['{0}'] config value - exiting.".format(val))

    return config_data

config = parse_config(args.config)

targets = config['targets']
interval = config['interval']
debug = args.debug
columns = config['columns']
db_host = config['db']['host']
db_port = config['db']['port']
db_user = config['db']['user']
db_pass = config['db']['pass']
db_name = config['db']['name']

if debug:
    verbose_requests()

try:
    client = influxdb.InfluxDBClient(db_host, db_port, db_user, db_pass, db_name)
except:
    print "Failed to set up client connection to InfluxDB"
    exit(1)

def get_metrics_json_from_fds(om_host):
    auth_token = requests.get('http://{0}/api/auth/token?login=admin&password=admin'.format(om_host)).json()['token']
    metrics_put_cookies = {"token": auth_token}
    metrics_put_payload = '{{"seriesType":["PUTS","GETS","SSD_GETS","QFULL","MBYTES","BLOBS","OBJECTS","ABS","AOPB","LBYTES","PBYTES","STC_SIGMA","LTC_SIGMA","STP_SIGMA","LTP_SIGMA","STC_WMA","STP_WMA"],"contextList":[],"range":{{"start":{0},"end":{1}}}}}'.format(start_time, end_time)
    metrics_json = requests.put('http://{0}/api/stats/volumes'.format(om_host), data=metrics_put_payload, cookies=metrics_put_cookies)

    return metrics_json.json()['series']

def reorg_metrics(cluster_name, raw_json):
    clean_json = []
    series_dict = {}

    for doc in raw_json:
        series_name = doc['type']
        if not series_name in series_dict:
            series_dict[series_name] = []

        series_dict[series_name].append( [cluster_name, doc['context']['name'], doc['context']['id'], doc['datapoints'][0]['y'], doc['datapoints'][0]['x']] )

    for series_name in series_dict.keys():
        clean_json.append( {'name': series_name, 'columns': columns, 'points': series_dict[series_name] } )

    return clean_json

while True:
    end_time = int(time.time() - 600)
    start_time = end_time - interval
    for cluster, host in targets:
        influxdb_payload = reorg_metrics(cluster, get_metrics_json_from_fds(host))

        if len(influxdb_payload):
            print influxdb_payload
            try:
                client.write_points(influxdb_payload, 's')
            except:
                print "ERROR: Could not write points to InfluxDB"
            else:
                print "Successfully wrote points to InfluxDB"
        else: print "No data to write to InfluxDB"

    time.sleep(float(interval))

