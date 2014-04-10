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

    def getActivePackages(self,pkgs=None):
        cmd=['dpkg', '--get-selections']
        cmd.extend(self.getAsList(pkgs))
        return [line.split()[0] for line in self.getCmdOutput(cmd) if line.split()[1] == 'install']

    def getPackageList(self,pkgs=None,version=False):
        pkgs=self.getActivePackages(pkgs)

        if not version:
            return pkgs

        cmd=['dpkg-query', '-f' ]
        cmd.append('${Package}=${Version}=${db:Status-Abbrev}\n')
        cmd.append('-W')
        cmd.extend(pkgs)
        lines=self.getCmdOutput(cmd)
        pkgs=[]
        for line in lines:
            
            pkg,ver,status = line.split('=')
            if status.strip() == 'ii':
                if len(ver)>0:
                    pkgs.append('%s=%s' % (pkg,ver))
                else:
                    pkgs.append(pkg)
        return pkgs

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

    def runCmd(self, cmd):
        log.debug('running cmd : %s', cmd)
        os.system(cmd)

    def update(self) :
         self.runCmd('apt-get update %s' %(Pkg.APTOPTIONS))

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


    def install(self,pkgs):
        debs = [p for p in pkgs if p.endswith('.deb')]
        nondebs = [p for p in pkgs if not p.endswith('.deb')]
        
        if len(debs) > 0:
            self.runCmd( 'dpkg --install %s' % (' '.join(debs)))

        if len(nondebs) > 0:
            self.runCmd( 'apt-get install %s' % (' '.join(nondebs)))

    def uninstall(self,pkgs):
        debs = [ p.split('_')[0] for p in pkgs ]
        debs = map(lambda(x): x[:-4] if x.endswith('.deb') else x,debs)
        
        if len(debs) > 0:
            self.runCmd( 'dpkg --remove %s' % (' '.join(debs)))

    def checkPrivileges(self):
        if os.geteuid() != 0:
            log.error('this command needs root priviliges')
            return False
        return True

    def list(self,pkgs,files=False):
        if len(pkgs)==0:
            pkgs=['fds*']

        if files :
            pkglist=self.getPackageList(pkgs)
            for pkg in pkglist:
                self.printLines([ '%s:\t%s' % (pkg, f)  for f in self.getPackageFiles(pkg)])
        else:
            # check if the arg is a file or pkgname
            for item in pkgs:
                log.debug(item)
                if item.startswith('/'):
                    if os.path.exists(item):
                        self.printLines(self.getOwner(item))
                    else:
                        log.error('check filepath [%s]', item)
                else:
                    self.printLines(self.getPackageList(item,True))

def usage():
    return '''
%(prog)s cmd [options] [[pkgname]...] [file]
   cmd :-
       ls      : list packages
       info    : display pkg information
       install : install the given deb pkgs
       update  : update info for the fds repo
       help    : display this menu
   options :-
       --files  : for ls , list files in the pkg
       [file]   : for ls , if a file is specified, it will print the pkg that owns it
'''

if __name__ == "__main__":
    #print sys.argv
    #sys.exit(0)
    parser = argparse.ArgumentParser(description='Manage Pkg info', 
                                     usage = usage())
    #parser.add_argument('cmd',default='help' ,help='ls,info,which,help',nargs='?' )
    parser.add_argument('-f','--files', action="store_true")
    parser.add_argument('-v','--verbose', action="store_true")
    parser.add_argument('other', nargs='*')

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
    elif args.cmd in ['-h' , '--help' ]:
        cmd='help'

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
                pkg.list(args.other,args.files)
            elif cmd=='info':
                if len(args.other) == 0:
                    args.other.append('fds-*')
                pkgs = pkg.getPackageList(args.other)
                pkg.printLines(pkg.getPackageInfo(pkgs))
            elif cmd == 'update':
                if not pkg.checkPrivileges() : sys.exit(1)
                pkg.update()
            elif cmd == 'install':
                if not pkg.checkPrivileges() : sys.exit(1)
                if len(args.other) > 0:
                    pkg.install(args.other)
                else:
                    log.error('please specify packages to install')

            elif cmd in ['uninstall' ,'remove' ]:
                if not pkg.checkPrivileges() : sys.exit(1)
                if len(args.other) > 0:
                    pkg.uninstall(args.other)
                else:
                    log.error('please specify packages to install')
            else:
                log.error('unknown command : %s' , cmd)
            
        except KeyboardInterrupt:
            pass;
