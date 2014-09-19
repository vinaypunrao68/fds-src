#!/usr/bin/python3

import os, sys, re, json, random, pickle
from optparse import OptionParser


def uploaded(options):
    if os.path.exists(".uploaded.pickle"):
        prev_uploaded_file = open(".uploaded.pickle", "rb")
        upk = pickle.Unpickler(prev_uploaded_file)
        return upk.load()

class FdsJson:
    def __init__(self, options):
        self.options = options
        self.cnt = 0
        self.json = None
        self.uploaded = uploaded(options)

    def _gen_am_op(self):
        # name = "file" + str(self.cnt)
        # self.cnt += 1
        volume_id = random.randrange(self.options.num_volumes)
        name = "file" + str(random.sample(self.uploaded[volume_id], 1)[0])
        am_op = {
            #"blob-op":   "startBlobTx",
            "blob-op":   "getBlob",
		    "volume-name": "volume" + str(volume_id),
            "blob-name": name,
		    "blob-off":  0,
            "object-len": self.options.file_size
        }

        return am_op

    def gen_am(self):
        top = {
            "Run-Input" : {
                "am-workload" : {
                    "am-ops" : []
                }
            }
        }
        for i in range(self.options.n_reqs):
            am_op = self._gen_am_op()
            top["Run-Input"]["am-workload"]["am-ops"].append(am_op)
        self.json = top

    def dump(self):
        return json.dumps(self.json)

    @staticmethod
    def decode(filein):
        fin = open(filein, "r")
        upk = pickle.Unpickler(fin)
        uploaded = upk.load()
        fin.close()
        print ("Number of volumes:", len(uploaded))
        for i, e in enumerate(uploaded):
            print ("files per volume", i, len(e))

def main():
    parser = OptionParser()
    #parser.add_option("-d", "--directory", dest = "directory", help = "Directory")
    #parser.add_option("-d", "--node-name", dest = "node_name", default = "node", help = "Node name")
    #parser.add_option("-n", "--name-server-ip", dest = "ns_ip", default = "10.1.10.102", help = "IP of the name server")
    #parser.add_option("-p", "--name-server-port", dest = "ns_port", type = "int", default = 47672, help = "Port of the name server")
    #parser.add_option("-s", "--server-ip", dest = "s_ip", default = "10.1.10.18", help = "IP of the server")
    #parser.add_option("-c", "--column", dest = "column", help = "Column")
    #parser.add_option("-p", "--plot-enable", dest = "plot_enable", action = "store_true", default = False, help = "Plot graphs")
    #parser.add_option("-A", "--ab-enable", dest = "ab_enable", action = "store_true", default = False, help = "AB mode")
    #parser.add_option("-t", "--threads", dest = "threads", type = "int", default = 1, help = "Number of threads")
    parser.add_option("-n", "--num-requests", dest = "n_reqs", type = "int", default = 1, help = "Number of requests per thread")
    #parser.add_option("-T", "--type", dest = "req_type", default = "PUT", help = "PUT/GET/DELETE")
    parser.add_option("-s", "--file-size", dest = "file_size", type = "int", default = 4096, help = "File size in bytes")
    #parser.add_option("-F", "--num-files", dest = "num_files", type = "int", default = 10000, help = "Number of files")
    parser.add_option("-v", "--num-volumes", dest = "num_volumes", type = "int", default = 1, help = "Number of volumes")
    #parser.add_option("-u", "--get-reuse", dest = "get_reuse", default = False, action = "store_true", help = "Do gets on file previously uploaded with put")
    #parser.add_option("-b", "--heartbeat", dest = "heartbeat", type = "int", default = 0, help = "Heartbeat. Default is no heartbeat (0)")
    #parser.add_option("-N", "--target-node", dest = "target_node", default = "localhost", help = "Target node (default is localhost)")
    #parser.add_option("-P", "--target-port", dest = "target_port", type = "int", default = 8000, help = "Target port (default is 8000)")
    #parser.add_option("-V", "--volume-stats", dest = "volume_stats",  default = False, action = "store_true", help = "Enable per volume stats")
    parser.add_option("-j", "--json-file", dest = "json_file", default = None, help = "Json input file")

    (options, args) = parser.parse_args()

    if options.json_file != None:
        FdsJson.decode(options.json_file)
        os._exit(0)

    json = FdsJson(options)
    json.gen_am()
    print (json.dump())


if __name__ == "__main__":
    main()

