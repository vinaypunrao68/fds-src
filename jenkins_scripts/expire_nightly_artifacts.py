#!/usr/bin/python

from httplib2 import Http
import json
import re
import sys

artifacts_to_keep = 14
deb_dict = {}
deb_names = [ "fds-platform" ]

conn = Http()
conn.add_credentials('jenkins','UP93STXWFy5c')
resp, content = conn.request("http://artifacts.artifactoryonline.com/artifacts/api/storage/formation-apt/pool/nightly", "GET")

content_json = json.loads(content)

print "\nINFO: Expriring old artifacts from Artifactory (> 13 builds ago)"
        
for deb_name in deb_names:
    deb_dict[deb_name] = []

for record in content_json['children']:
    for deb_name in deb_names:
        if re.compile(".*" + deb_name + "_.*-(debug|release)-.*").match(record['uri']):
            deb_dict[deb_name].append(record['uri'])

        deb_dict[deb_name].sort(key=lambda deb: int(deb.split('-')[3].split('.')[0]))

print deb_dict
for deb_name in deb_dict.keys():
    if len(deb_dict[deb_name]) > artifacts_to_keep:
        for artifact in deb_dict[deb_name][:-artifacts_to_keep]:
            print "Deleting: " + artifact
            try:
                resp, content = conn.request("http://artifacts.artifactoryonline.com/artifacts/formation-apt/pool/nightly" + artifact, "DELETE")

            except:
                e = sys.exc_info()[0]
                print "ERROR submitting request: " + e.strerror
                continue

            print resp.status.__class__.__name__
            print resp.status

            if resp.status == 204:
                print "SUCCESS deleting artifact " + artifact
            else:
                err_message = json.loads(content)['errors'][0]['message']
                print "ERROR deleting artifact " + artifact + ": '" + str(resp.status) + " - " + err_message + "'"

    else:
        print "INFO : No expired artifacts for deb name: " + deb_name
