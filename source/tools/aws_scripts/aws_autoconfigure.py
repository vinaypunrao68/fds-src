#!/usr/bin/python
#
# Tool for automatically setting up the AWS CLI with the proper secret key
#
import sys
import requests
import json
import getpass
import argparse
import urlparse
import subprocess

parser = argparse.ArgumentParser(description='automatically set up AWS config')
parser.add_argument('-u', help='username')
parser.add_argument('-p', help='password (blank for prompt)')
parser.add_argument('-om', required=True, help='OM host')
parser.add_argument('-d', action='store_true', dest='debug', help='debug')
parser.add_argument('-print', action='store_true', dest='print_key', help='output key instead of configuring AWS')
args = parser.parse_args()

username = args.u
if username is None:
    print "Username: ",
    username = sys.stdin.readline().strip()

password = args.p
if password is None:
    password = getpass.getpass().strip()


om_host = args.om
#todo - add url detection
base_url = "https://" + om_host + ":7443/api/auth/token"  
print "getting token from " + base_url

full_url = base_url + "?login=" + username + "&password=" + password
response = requests.get(full_url, verify=False)

if args.debug:
    print response.json()
token = response.json()["token"]

if not args.print_key:
    sub = subprocess.Popen(['aws', 'configure'], stdin=subprocess.PIPE, stdout=None)
    sub.communicate(username + '\n' + token + "\n" + "\n" + "\n")
else:
    print token

print "\n"
