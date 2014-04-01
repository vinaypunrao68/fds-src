#!/usr/bin/env python
import argparse
import subprocess
import types
import sys
import os
import logging

logging.basicConfig(level=logging.INFO,format="%(levelname)s : %(name)s : %(message)s")
log = logging.getLogger('pkg')
log.setLevel(logging.INFO)

class Pkg:
    REPOFILE='/etc/apt/sources.list.d/fds.list'
    APTOPTIONS='-o Dir::Etc::sourcelist="sources.list.d/fds.list" -o Dir::Etc::sourceparts="-" -o APT::Get::List-Cleanup="0"'

    def __init__(self):
        self.verbose = False

    def getPackageList(self,pkgs=None):
        cmd=['dpkg', '--get-selections']
        cmd.extend(self.getAsList(pkgs))
        lines=self.getCmdOutput(cmd)
        return [line.split('\t')[0] for line in lines if -1==line.find('deinstall')]

    def getPackageFiles(self,pkgname):
        cmd= ['dpkg-query', '-L']
        cmd.extend(self.getAsList(pkgname))
        return self.getCmdOutput(cmd)
    
    def getOwner(self,files):
        cmd= ['dpkg', '-S']
        cmd.extend(self.getAsList(files))
        return self.getCmdOutput(cmd)                   

    def getPackageInfo(self,pkgs):
        cmd= ['dpkg', '-p']
        cmd.extend(self.getAsList(pkgs))
        return self.getCmdOutput(cmd)

    def update(self) :
        cmd = 'apt-get update %s' %(Pkg.APTOPTIONS)
        os.system(cmd)

    def getAsList(self,data):
        if type(data)==types.ListType:
            return data
        elif type(data)==types.StringType:
            return [data]
        return []

    def getCmdOutput(self,cmd):
        log.debug(cmd)
        p1=subprocess.Popen(cmd,stdout=subprocess.PIPE)
        return [line.strip('\n') for line in p1.stdout]

    def printLines(self,lines):
        for line in lines:
            print line


def usage():
    return '''
%(prog)s cmd [options] [[pkgname]...] [file] [program-options]
   cmd :-
       ls    : list packages
       info  : display pkg information
       update: update info for the fds repo
       help  : display this menu
   options :-
       --files  : for ls , list files in the pkg
       [file]   : for ls , if a file is specified, it will print the pkg that owns it
   program-options:
             : all these will be sent to the subprocess
'''

if __name__ == "__main__":

    #print sys.argv
    #sys.exit(0)
    parser = argparse.ArgumentParser(description='Manage Pkg info', 
                                     usage = usage())
    #parser.add_argument('cmd',default='help' ,help='ls,info,which,help',nargs='?' )
    parser.add_argument('-f','--files', action="store_true")
    parser.add_argument('-v','--verbose', action="store_true")
    parser.add_argument('other', nargs=argparse.REMAINDER)

    cmd='help'
    if len(sys.argv) >1 :
        cmd= sys.argv[1]
        del sys.argv[1]

    args = parser.parse_args()
    args.cmd=cmd

    if args.cmd=='ls' or args.cmd=='list':
        cmd='list'
    elif args.cmd=='info':
        cmd='info'

    if args.verbose:
        log.setLevel(logging.DEBUG)

    log.debug('cmd=%s', cmd)

    if cmd=='help':
	parser.print_usage()
    else:
        try :
            pkg=Pkg()
            pkg.verbose = args.verbose
            if cmd == 'list':
                if args.files :
                    pkglist=pkg.getPackageList(args.other)
                    for p in pkglist:
                        pkg.printLines([ p + ':\t' + f  for f in pkg.getPackageFiles(p)])
                else:
                    # check if the arg is a file or pkgname
                    for item in args.other:
                        log.debug(item)
                        if item.startswith('/'):
                            if os.path.exists(item):
                                pkg.printLines(pkg.getOwner(item))
                            else:
                                log.error('check filepath [%s]', item)
                        else:
                            pkg.printLines(pkg.getPackageList(item))

                    if len(args.other)==0:
                        pkg.printLines(pkg.getPackageList('fds*'))

            elif cmd=='info':
                pkg.printLines(pkg.getPackageInfo(args.other))
            elif cmd == 'update':
                pkg.update()
            else:
                log.error('unknown command : %s' , cmd)
            
        except KeyboardInterrupt:
            pass;
