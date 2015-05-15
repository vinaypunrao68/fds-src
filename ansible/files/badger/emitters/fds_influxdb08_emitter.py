import json
import urllib2

class fds_influxdb08_emitter(object):
    def __init__(self, config=None, logger=None):
        self.log = logger
        self.config = config

        if not self.config['interval']:
            self.interval = 1
        else:
            self.interval = self.config['interval']

        self.columns = ['value', 'units', 'time', 'datacenter', 'region', 'zone', 'cluster', 'hostname', 'ipv4', 'ipv6']
        self.host = self.config['host']
        self.port = self.config['port']
        self.database = self.config['database']
        self.user = self.config['user']
        self.password = self.config['pass']

        if self.log == None:
            import logging
            self.log = logging.getLogger(__name__)
            self.log.setLevel(logging.DEBUG)
            self.log.addHandler(logging.StreamHandler())

        self.url = "http://{0}:{1}/db/{2}/series?u={3}&p={4}&time_precision=s".format(self.host, self.port, self.database, self.user, self.password)
        self.log("debug", msg=self.url)

    def emit_stats(self, payload, global_iteration):
        # take into account custom interval, if present in config
        if global_iteration % self.interval:
            return

        influxdb_payload = []
        timestamp = int(payload['timestamp'])
        datacenter = payload['datacenter']
        region = payload['region']
        zone = payload['zone']
        cluster = payload['cluster']
        hostname = payload['hostname']
        ipv4 = payload['ipv4']
        ipv6 = payload['ipv6']

	# This emitter is custom-designed for FDS use. If the stuff in here looks weird,
	#  that's why.
        for series_data in payload['points']:
            series_name = series_data.keys()[0]

            # when dealing with FDS counters, we want to yank the 'volume.N' stuff
            #  out of the series name and instead use it as a field in the InfluxDB record
            if series_name[:4] == "fds.":
                split_name = series_name.split(".")
                if split_name[-1] == "count" or split_name[-1] == "latency":
                    stat_type = split_name.pop()
                    volume_id = split_name.pop()
                    split_name.append(stat_type)
                else:
                    volume_id = split_name.pop()
                fds_series_name = ".".join(split_name)
#                influxdb_payload.append(series_name)
                influxdb_payload.append({
                    'name': fds_series_name,
                    'columns': self.columns + ['volume_id'],
                    'points': [[series_data[series_name]['value'], series_data[series_name]['units'], timestamp, datacenter, region, zone, cluster, hostname, ipv4, ipv6, volume_id]]
                })
            else:
                influxdb_payload.append({
                    'name': series_name,
                    'columns': self.columns,
                    'points': [[series_data[series_name]['value'], series_data[series_name]['units'], timestamp, datacenter, region, zone, cluster, hostname, ipv4, ipv6]]
                })

#        print json.dumps(influxdb_payload, sort_keys=True, indent=4, separators=(',', ': '))
#        for i in influxdb_payload:
#            print i

        req = urllib2.Request(self.url, json.dumps(influxdb_payload), {'Content-Type': 'application/json'})

        try:
            urllib2.urlopen(req)
        except urllib2.HTTPError as e:
            self.log("err", msg="InfluxDB ingest request failed. Response: {0}".format(e), error="EmitFailed")
        else:
            self.log("debug", msg="Successfully sent batch to InfluxDB")
