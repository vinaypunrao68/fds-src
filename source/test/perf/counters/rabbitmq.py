#!/usr/bin/env python
import pika
import json
import time
import sys
import re
sys.path.append("../common")
import utils

class RabbitMQClient(object):

    def __init__(self, host='localhost', port=5672, username='guest', passwd='guest', period=5):
        self.connection = pika.BlockingConnection(pika.ConnectionParameters(
                                                        host=host, 
                                                        port=port, 
                                                        credentials=
                                                            pika.credentials.PlainCredentials(username, passwd))
                                                        )
        self.channel = self.connection.channel()
        self.channel.queue_declare(queue='stats.work')
        self.period = period

    # publish a data point
    def publish(self, name, period, value, volume_id, service, node, cntr_type, timestamp):
        if not 'min_value' in vars(self):
            self.min_value = value
        else:
            self.min_value = min(value, self.min_value)
        if not 'max_value' in vars(self):
            self.max_value = value
        else:
            self.max_value = max(value, self.max_value)
        data = [
                {
                "minimumValue"          :   self.min_value, 
                "maximumValue"          :   self.max_value,
                "collectionTimeUnit"    :   "SECONDS",
                "metricName"            :   name,
                "collectionPeriod"      :   period,
                "contextType"           :   "VOLUME",
                "metricValue"           :   value,
                "contextId"             :   volume_id,
                "numberOfSamples"       :   1,
                "relatedContexts"       :   [
                                                {"contextType": "SERVICE", "contextId" : service}, 
                                                {"contextType": "NODE", "contextId" : node}, 
                                                {"contextType": "CNTR_TYPE", "contextId" : cntr_type}
                                            ],
                "reportTime"            :   timestamp
                }
        ]
        json_data = json.dumps(data)
        self.channel.basic_publish(exchange='',
                      routing_key='stats.work',
                      body=json_data)
        # print " [x] Sent ->", json_data

    # close connection    
    def close(self):
        self.connection.close()

    # write a series of metrics    
    def write_records(self, series, records):
        # series: <service>.<node>
        service, node = series.split('.', 1)
        # records: lost of (name.<volume_id>[.<cntr_type>], value)
        cols, vals = [list(x) for x in  zip(*records)]
        vals = [utils.dyn_cast(x) for x in vals]
        n = len(cols)
        for i in range(n):
            m = re.match("(\w*)\.(\w*)(?:\.(\w*))?", cols[i])
            if m != None:
                tokens = m.groups()
                name = tokens[0]
                volume_id = int(tokens[1])
                cntr_type = tokens[2]
                timestamp = int(round(time.time() * 1000))
                self.publish(name, self.period, vals[i], volume_id, service, node, cntr_type, timestamp)
        

def main():

    # testing stuff 

    client = RabbitMQClient()
    client.publish("AM_GET_REQ", 5, 33300, 22, "am", "mynode", "count", int(round(time.time() * 1000)))
    series = "am.10.1.10.34"
    records = [ 
                ("AM_GET_REQ.5.count", 33), 
                ("AM_GET_REQ.5.latency", 33888.03), 
                ("AM_CACHE_MISS.5", 338001), 
                ("AM_GET_REQ.4.count", 32), 
                ("AM_GET_REQ.4.latency", 132288.03), 
                ("AM_CACHE_MISS.4", 001), 
              ]
    client.write_records(series, records)
    client.close()

if __name__ == "__main__":
    main()

