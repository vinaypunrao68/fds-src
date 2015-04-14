#!/usr/bin/python
from BaseHTTPServer import BaseHTTPRequestHandler
import BaseHTTPServer
from SocketServer import ThreadingMixIn
import os
import subprocess
import urlparse
import sys
import json


class NbdRequestHandler(BaseHTTPRequestHandler):
    def nbdadm_path(self):
        return os.path.join(os.path.dirname(sys.argv[0]), "nbdadm.py")

    def run(self, cmd):
        proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, close_fds=True)
        (stdout, stderr) = proc.communicate()
        return (proc.returncode, stdout, stderr)

    def write_output(self, result):
        (exit_code, stdout, stderr) = result
        self.send_response(200)
        self.send_header("Content-type:", "application/json")
        self.end_headers()

        if exit_code == 0:
            json.dump({"result": 'ok', "output": stdout}, self.wfile)
        else:
            json.dump({"result": 'fail', "output": stdout, "error_code": exit_code, "error": stderr}, self.wfile)

        self.wfile.flush()
        self.finish()


    def do_GET(self):
        parse_result = urlparse.urlparse(self.path)
        params = urlparse.parse_qs(parse_result.query)
        verb = params.get('op', [None])[0]
        if verb == 'attach' and 'host' in params and 'volume' in params:
            self.write_output(self.run([self.nbdadm_path(), 'attach', params['host'][0], params['volume'][0]]))
        elif verb == 'detach' and 'host' in params and 'volume' in params:
            self.write_output(self.run([self.nbdadm_path(), 'detach', params['host'][0], params['volume'][0]]))
        elif verb == 'detach' and 'volume' in params:
            self.write_output(self.run([self.nbdadm_path(), 'detach', params['volume'][0]]))
        else:
            self.send_error(400, 'bad request')


server = BaseHTTPServer.HTTPServer(('', 10511), NbdRequestHandler)
server.serve_forever()
