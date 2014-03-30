#!/usr/bin/env python
import argparse
import sys
import os
import requests
import logging
import re

logging.basicConfig(level=logging.WARN,format="%(levelname)s : %(name)s : %(message)s")
log = logging.getLogger('pkgdist')
log.setLevel(logging.INFO)

class PkgDist:
    def __init__(self):
        self.nameformat=re.compile("(.*)_([a-z0-9.-]+).deb")
        self.args = None

    def verify(self):
        if len(self.args.pkg) == 0:
            log.error("please specify a pkg")
            return False
        
        for pkg in self.args.pkg:
            filename = os.path.basename(pkg)

            match = self.nameformat.match(filename)
            if not match:
                log.error("unknown file format : %s",filename)
                return False;

            # now verify the file .
            execcmd="lintian -C deb-format " + pkg + " 2>/dev/null"
            log.debug(execcmd)
            ret=os.system(execcmd)
            if ret != 0 :
                log.error("pkg not in debian format %s" , pkg)
                return False
            
        return True

    def process(self):
        for pkg in self.args.pkg:
            filename = os.path.basename(pkg)
            url = 'http://%s?filename=%s&branch=%s' %(self.args.repo, filename, self.args.branch)
            try :
                resp = requests.post(url,data=open(pkg).read())
                if resp.status_code != 200:
                    log.error("pkg upload failed for [%s]" , pkg)
                    log.error("error msg : [%s]" , resp.reason)
                else :
                    log.info("pkg upload success for [%s]" , pkg)
            except requests.ConnectionError:
                log.error("pkg upload failed for [%s]" , pkg)
                log.error("unable to communicate with repo : %s" , url)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Upload a a pkg to fds repo',formatter_class=argparse.ArgumentDefaultsHelpFormatter) 
    parser.add_argument('-r','--repo', default='coke.formationds.com:8081', help='Repository location')
    parser.add_argument('-b','--branch', default='test', help='Branch' ,"upload to specific branch [test/stable]")
    parser.add_argument('-v','--verbose', action="store_true" , help = 'be more verbose')
    parser.add_argument('-n','--dryrun', action="store_true" , 'just check , dont upload')
    parser.add_argument('pkg', nargs=argparse.REMAINDER)

    args = parser.parse_args()
    dist = PkgDist()
    
    if args.verbose:
        log.setLevel(logging.DEBUG)
    
    if len(args.pkg) == 0:
        log.error("please specify atleast one pkg")
        parser.print_help()
        sys.exit(1)

    dist.args = args
    if dist.verify():
        if not dist.args.dryrun:
            log.info('not uploading as this is a dryrun')
        else:
            dist.process()

    
