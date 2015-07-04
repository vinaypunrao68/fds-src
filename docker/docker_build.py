#!/usr/bin/env python
#
# Build Docker images
#

from docker import Client
import json
import re
import os
import sys
import argparse

IMAGES = {
        'base': {
            'tag':'registry.formationds.com:5000/fds_base',
            'path':'base_image'
        },
        'dev': {
            'tag':'registry.formationds.com:5000/fds_dev',
            'path':'dev_image'
        }
}

HOSTS = {
    'build': 'tcp://fre-build-04.formationds.com:2375',
    'pull': [
        'tcp://fre-build-01.formationds.com:2375',
        'tcp://fre-build-02.formationds.com:2375',
        'tcp://fre-build-03.formationds.com:2375',
        'tcp://bld-dev-01.formationds.com:2375',
        'tcp://bld-dev-02.formationds.com:2375'
    ]
}

class FDSDocker():

    def __init__(self, opts):
        self.images = IMAGES
        self.hosts = HOSTS
        self.buildhost = opts['buildhost']
        self.opts = opts
        self.debug = opts['debug']

    def get_buildhost(self):
        """Get host URL for building - returns string"""
        return self.buildhost

    def get_pullhosts(self):
        """Get list of hosts to pull image to - returns list"""
        return self.hosts['pull']

    def get_imageattrs(self, name):
        """Return dict of image attributes"""
        return self.images[name]

    def get_conn(self, url):
        """Return docker client object"""
        return Client(base_url=url, version='auto')

    def parse_stream(self, stream, tag='stream'):
        """Parse docker stream json output into raw messages"""
        for line in stream:
            if self.debug:
                print "D: " + line,
            else:
                try:
                    print json.loads(line).get(tag),
                except ValueError:
                    print [
                        json.loads(obj).get(tag, '') for obj in
                        re.findall('{\s*"' + tag + '"\s*:\s*"[^"]*"\s*}', line)
                    ],

    def parse_status(self, stream, tag='status'):
        """Parse docker stream json output into raw messages
           Unfortunately this format includes newlines unlike the stream
           output - so yay! we get to duplicate to avoid stripping newlines"""
        for line in stream:
            if self.debug:
                print "D: " + line
            else:
                if re.search('(Downloading|Buffering|Pushing)', line):
                    print ".",
                else:
                    try:
                        print json.loads(line).get(tag)
                    except ValueError:
                        print [
                            json.loads(obj).get(tag, '') for obj in
                            re.findall('{\s*"' + tag + '"\s*:\s*"[^"]*"\s*}', line)
                        ]

    def build(self):
        """Build a docker image"""
        print "Building image for {}".format(self.opts['type'])
        image = self.get_imageattrs(self.opts['type'])
        buildclient = self.get_conn(self.get_buildhost())
        build_result = buildclient.build(path=image['path'],
                                tag=image['tag'],
                                nocache=True,
                                pull=True,
                                stream=True
                               )
        self.parse_stream(build_result)

    def pushimage(self):
        """Push an image to the docker repository"""
        image = self.get_imageattrs(self.opts['type'])
        pushclient = self.get_conn(self.get_buildhost())
        push_response = pushclient.push(repository=image['tag'],
                                        stream=True,
                                        insecure_registry=True)
        self.parse_status(push_response)

    def pullimages(self):
        """Pull built image to all hosts"""
        img_tag = self.get_imageattrs(self.opts['type'])['tag']
        for host in self.get_pullhosts():
            print "Pulling {} to {}".format(img_tag, host)
            c = self.get_conn(host)
            pull_result = c.pull(repository=img_tag,
                                 stream=True,
                                 insecure_registry=True
                                )
            self.parse_status(pull_result)

    def listtypes(self):
        print "Images available to build:"
        for image in self.images:
            print "{}: Name: {} Dir: {}".format(image,
                                                self.images[image]['tag'],
                                                self.images[image]['path'])

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
            '--type', dest='type', default='base',
            help='image to build')
    parser.add_argument(
            '--buildhost', dest='buildhost',
            default='tcp://fre-build-04.formationds.com:2375',
            help='Modify the default host where images are built')
    parser.add_argument(
            '--listtypes', action='store_true', default=False,
            help='List image types to build')
    parser.add_argument(
            '--debug', action="store_true", default=False,
            help='enable debugging')
    parser.add_argument(
            '--buildonly', action="store_true", default=False,
            help='Only build')
    parser.add_argument(
            '--pullonly', action="store_true", default=False,
            help='Only pull, do not build')
    parser.add_argument(
            '--pushonly', action="store_true", default=False,
            help='Only push, do not build')

    args = parser.parse_args()
    opts = vars(args)

    buildclient = FDSDocker(opts)

    if opts['listtypes'] is True:
        buildclient.listtypes()
    elif opts['buildonly'] is True:
        buildclient.build()
    elif opts['pullonly'] is True:
        buildclient.pullimages()
    elif opts['pushonly'] is True:
        buildclient.pushimage()
    else:
        buildclient.build()
        buildclient.pushimage()
        buildclient.pullimages()

if __name__ == "__main__":
    main()
