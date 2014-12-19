#!/usr/bin/env python
# Submit workloads to cosbench

import argparse
import subprocess
import requests
from jinja2 import Template

class COSBench:

    def __init__(self, args):
        self.debug = args.debug
        self.fdshost = args.fdshost
        self.fdsuser = args.fdsuser
        self.fdspass = args.fdspass
        self.authport = args.authport
        self.s3port = args.s3port
        self.chost = args.chost
        self.cport = args.cport
        self.cuser = args.cuser
        self.cpass = args.cpass
        self.workload = args.workload
        self.validate = args.validate

    def get_user_token(self):
        url = self.get_auth_url()
        print("Getting credentials from: ", url)
        r = requests.get(url, verify=self.validate)
        rjson = r.json()
        print(rjson)
        return rjson['token']

    def get_s3_url(self):
        return "http://%s:%s" % (self.fdshost, self.s3port)

    def get_auth_url(self):
        return 'http://%s:%s/api/auth/token?login=%s&password=%s' % (
                self.fdshost,
                self.authport,
                self.fdsuser,
                self.fdspass)

    def get_cosbench_url(self):
        return 'http://%s:%s/controller/cli/submit.action?username=%s&password=%s' % (
                self.chost,
                self.cport,
                self.cuser,
                self.cpass)

    def parse_template(self):
        auth_url = self.get_auth_url()
        s3_url = self.get_s3_url()
        token = self.get_user_token()
        file = open(self.workload, 'r')
        t = Template(file.read())
        return t.render(username=self.fdsuser,secretkey=token,s3_url=s3_url)

    def submit_workload(self, xml):
        url = self.get_cosbench_url()
        print "XML:"
        print xml
        print "Posting workload to cosbench..."
        print "Posting to %s" % url
        file = {'config': ('workload.xml', xml)}
        r = requests.post(url, files=file)
        print r.status_code
        print r.text

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--debug', action='store_true', default=False,
                        help='enables debug mode')
    parser.add_argument('--fdshost', default='127.0.0.1',
                        help='Host running FDS ')
    parser.add_argument('--chost', default='127.0.0.1',
                        help='Host running COSBench Controller')
    parser.add_argument('--cport', default='19088',
                        help='COSBench Controller port')
    parser.add_argument('--cuser', default='',
                        help='COSBench User')
    parser.add_argument('--cpass', default='',
                        help='COSBench Pass')
    parser.add_argument('--authport', default='7777',
                        help='FDS auth port')
    parser.add_argument('--s3port', default='8000',
                        help='FDS s3 port')
    parser.add_argument('--fdsuser', default='admin',
                        help='FDS auth username')
    parser.add_argument('--fdspass', default='admin',
                        help='FDS auth password')
    parser.add_argument('--workload', default='',
                        help='File to use for workload template')
    parser.add_argument('--validate', default=False,
                        help='Do we validate SSL certs?')
    args         = parser.parse_args()

    cb = COSBench(args)
    cb.submit_workload(cb.parse_template())

if __name__ == '__main__':
    main()
