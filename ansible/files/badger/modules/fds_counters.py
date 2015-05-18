import json
import sys
import os
from datetime import datetime as dt
import time

class fds_counters(object):
    def __init__(self, config=None, logger=None):
        self.prefix = "fds."
        self.log = logger
        self.config = config
        self.service_table = {}

        if not self.config['interval']:
            self.interval = 1
        else:
            self.interval = self.config['interval']

        if self.log == None:
            import logging
            self.log = logging.getLogger(__name__)
            self.log.setLevel(logging.DEBUG)
            self.log.addHandler(logging.StreamHandler())

        for val in ['fds_root', 'om_ip', 'node_ip']:
            if not val in self.config:
                self.log("error", msg="fds_counters module config is missing '{}' item - Failed to initialize".format(val))
                raise Exception("Failed to initialize fds_counters", "missing config item")

        # Grab the SvcMap class so we can avoid re-importing all this jazz every polling interval
        sys.path.append(os.path.join(self.config['fds_root'], "lib/python2.7/dist-packages/fdslib/pyfdsp"))
        sys.path.append(os.path.join(self.config['fds_root'], "lib/python2.7/dist-packages/fdslib"))
        sys.path.append(os.path.join(self.config['fds_root'], "lib/python2.7/dist-packages"))
        sys.path.append("/opt/fds-deps/embedded/lib/python2.7/site-packages")
        self.SvcHandle = __import__("SvcHandle", fromlist=["SvcMap"])
        self.SvcMap = self.SvcHandle.SvcMap

    def update_service_map(self, global_iteration):
        try:
            self.service_map = self.SvcMap(self.config['om_ip'], 7020)
        except:
            import traceback
            ei = sys.exc_info()
            traceback.print_exception(ei[0], ei[1], ei[2], None, sys.stderr)
            return

        self.service_table = {}

        for entity in self.service_map.list():
            if entity[3] == self.config['node_ip'] and entity[5] == "Active":
                self.service_table[entity[1]] = entity[0]


    def get_stats(self, global_iteration):
        # take into account custom interval, if present in config
        if global_iteration % self.interval:
            return None

        try:
            self.update_service_map(global_iteration)
        except:
            import traceback
            ei = sys.exc_info()
            traceback.print_exception(ei[0], ei[1], ei[2], None, sys.stderr)
            return []

        payload = []

        for service_name, service_uuid in self.service_table.iteritems():
            counters = self.service_map.client(service_uuid).getCounters('*')

            for stat, val in counters.iteritems():
                if stat[:2].lower() in ['am', 'dm', 'om', 'pm', 'sm']:
                    stat_name = stat.replace(":", ".").replace("=", ".").replace("_", ".").lower()

                    if "latency" in stat_name:
                        payload.append({self.prefix + stat_name: {"value": val, "units": "ns"}})
                    else:
                        payload.append({self.prefix + stat_name: {"value": val, "units": ""}})

        return payload

if __name__ == "__main__":
    f = fds_counters(config={"interval":0, "blacklist": [], "fds_root": "/fds", "node_ip": "10.2.10.190", "om_ip": "10.2.10.190"})
    global_iteration = 0
    while (True):
        start=dt.now()
        print json.dumps(f.get_stats(global_iteration), sort_keys=True, indent=4, separators=(',', ': '))
        end=dt.now()
        print "Elapsed time to collect stats: {}".format((end - start).total_seconds())
        global_iteration += 1
        time.sleep(1)
