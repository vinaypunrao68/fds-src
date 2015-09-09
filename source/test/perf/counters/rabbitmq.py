#!/usr/bin/env python
import pika
import json
import time
import sys
import re
sys.path.append("../common")
import utils

millis = lambda: int(round(time.time() * 1000))

class RabbitMQClient(object):
    def __init__(self, period):
        self.connection = pika.BlockingConnection(pika.ConnectionParameters(
               'localhost'))
        self.channel = self.connection.channel()
        self.channel.queue_declare(queue='hello')
        self.period = period
    # service, node
    # name, volume_id, cntr_type
    def publish(self, name, period, value, volume_id, timestamp):
        data = [
                {
                "minimumValue":0, 
                "collectionTimeUnit"    :   "SECONDS",
                "metricName"            :   name,
                "collectionPeriod"      :   period,
                "contextType"           :   "VOLUME",
                "metricValue"           :   value,
                "contextId"             :   volume_id,
                "numberOfSamples"       :   1,
                "maximumValue"          :   0,
                "reportTime"            :   timestamp
                }
        ]
        json_data = json.dumps(data)
        self.channel.basic_publish(exchange='',
                      routing_key='metrics',
                      body=json_data)
        print " [x] Sent ->", json_data
    def close(self):
        self.connection.close()
    def write_records(self, series, records):
        # series: <service>.<node>
        service, node = series.split('.', 1)
        print service, node
        # records: lost of (name.<volume_id>[.<cntr_type>], value)
        cols, vals = [list(x) for x in  zip(*records)]
        vals = [utils.dyn_cast(x) for x in vals]
        n = len(cols)
        for i in range(n):
            m = re.match("(\w*)\.(\w*)(?:\.(\w*))?", cols[i])
            assert m != None, cols[i]
            tokens = m.groups()
            print cols[i], tokens
            name = tokens[0]
            volume_id = int(tokens[1])
            cntr_type = tokens[2]
            timestamp = millis()
            self.publish("AM_GET_REQ", self.period, vals[i], volume_id, timestamp)
        

def main():
    client = RabbitMQClient(5)
    timestamp = millis()
    client.publish("AM_GET_REQ", 5, 33300, 22, timestamp)
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
#    public String toJson(){
#        
#        JSONObject json = new JSONObject();
#        
#        json.put( REPORT_TIME, getReportTime() );
#        json.put( METRIC_NAME, getMetricName() );
#        json.put( METRIC_VALUE, getMetricValue() );
#        json.put( COLLECTION_PERIOD, getCollectionPeriod() );
#        json.put( CONTEXT_ID, getContextId() );
#        json.put( CONTEXT_TYPE_STR, getContextType().name() );
#        json.put( COLLECTION_TIME_UNIT, getCollectionTimeUnit().name() );
#        json.put( NUMBER_OF_SAMPLES, getNumberOfSamples() );
#        json.put( MINIMUM_VALUE, getMinimumValue() );
#        json.put( MAXIMUM_VALUE, getMaximumValue() );
#        
#        String rtn = json.toString();
#        return rtn;
#    }
