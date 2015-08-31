#!/usr/bin/python
import BaseHTTPServer
import SocketServer
import socket
from optparse import OptionParser
import threading
import signal
import sys
import re
import json
import dataset
import logging
sys.path.append("../common")
import utils

def get_data(name):
    connection = "mysql://perf@matteo-vm/experiments"
    logging.debug(connection)
    db = dataset.connect(connection)
    try:
        res = db[name].all()
        return list(res)
    except:
        return []
    
def search_data(name, tags):
    connection = "mysql://perf@matteo-vm/experiments"
    logging.debug(connection)
    db = dataset.connect(connection)
    # table = db["experiments"].table
    myquery = "SELECT * FROM " + name
    if len(tags) > 0:
        myquery = myquery + " WHERE "
    for i, t in enumerate(tags):
        if utils.is_int(t[1]) or utils.is_float(t[1]):
            myquery = myquery + t[0] + "=" + str(t[1])
        else:
            myquery = myquery + t[0] + "='" + t[1] + "'"
        if i < (len(tags) -1):
            myquery = myquery + " AND "
    try:
        logging.debug(myquery)
        res = db.query(myquery)
        return list(res)
    except:
        return []

class MyHandler(BaseHTTPServer.BaseHTTPRequestHandler):
    def do_GET(s):
        """Respond to a GET request."""
        s.send_response(200)
        s.send_header("Content-type", "application/json")
        s.send_header("Access-Control-Allow-Origin", "*");
        s.send_header("Access-Control-Expose-Headers", "Access-Control-Allow-Origin");
        s.send_header("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
        s.end_headers()
        m = re.match("(.*)\?(.*)", s.path)
        if m != None:
            # search
            name = m.group(1)[1:]
            tags = m.group(2).split('+')
            tags = [x.split('=') for x in tags]
            logging.debug(name + " " + str(tags))
            data = search_data(name, tags)
        else:
            data = get_data(s.path[1:])
        logging.debug(data)
        s.wfile.write(json.dumps(data))

class MyServer(object):
    def __init__(self, options):
        self.options = options
    def run(self):
        self.httpd = SocketServer.TCPServer(("", self.options.host_port), MyHandler)

        try:
            self.httpd.serve_forever()
        except socket.error as e:
            if e.errno != 9:
                raise e

    def terminate(self):
        self.httpd.socket.close();

def main():
    parser = OptionParser()
    parser.add_option("-H", "--host-ip", dest = "host_ip", default = "luke", help = "Host IP")
    parser.add_option("-P", "--host-port", dest = "host_port", type = "int", default = 44444,
                      help = "Host port")

    (options, args) = parser.parse_args()
    logging.basicConfig(level=logging.INFO)
    logging.info("Options:" + str(options))
    
    s = MyServer(options)

    t = threading.Thread(target = s.run)
    t.start()

    def signal_handler(signal, frame):
        print '... Exiting...'
        s.terminate()
        sys.exit(0)

    signal.signal(signal.SIGINT, signal_handler)
    print 'Press Ctrl+C to exit.'
    signal.pause()

if __name__ == "__main__":
    main()
