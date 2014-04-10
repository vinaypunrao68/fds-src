#!/usr/bin/env python

import shelve
import os
import logging
import re
import readline
import types

logging.basicConfig(level=logging.INFO,format="%(levelname)s : %(message)s")
log = logging.getLogger('pkg')
log.setLevel(logging.DEBUG)

class Color:
    END = '\033[0m'
    
    BLACK   = '\033[30m'
    RED     = '\033[31m'
    GREEN   = '\033[32m'
    YELLOW  = '\033[33m'
    BLUE    = '\033[34m'
    MAGENTA = '\033[35m'
    CYAN    = '\033[36m'
    WHITE   = '\033[37m'

    BOLDBLACK   = '\033[1;30m'
    BOLDRED     = '\033[1;31m'
    BOLDGREEN   = '\033[1;32m'
    BOLDYELLOW  = '\033[1;33m'
    BOLDBLUE    = '\033[1;34m'
    BOLDMAGENTA = '\033[1;35m'
    BOLDCYAN    = '\033[1;36m'
    BOLDWHITE   = '\033[1;37m'


class Installer:
    def __init__(self):
        self.datafile = '/tmp/fdsinstall.data'
        self.data = shelve.open(self.datafile,writeback=True)
        self.IP=re.compile('([0-9]{1,3}\.){3}[0-9]{1,3}')
        self.initMenu()

    def initMenu(self):
        self.menu = []
        self.menu.append([0,"Run All Steps"             ,'stepRunAll'         ])
        self.menu.append([1,"Configure Node"            ,'stepConfigureNode'  ])
        self.menu.append([2,"Run System Check"          ,'stepSystemCheck'    ])
        self.menu.append([3,"Check package dependencies",'stepDependencyCheck'])
        self.menu.append([4,"Install FDS Packages"      ,'stepInstallPackages'])
        
        # set the correct callback functions
        for item in self.menu:
            item.append(getattr(self,item[2]))

    def save(self):
        self.data.sync()

    def close(self):
        self.data.close()

    def confirm(self,message):
        ans=''
        while ans.lower() not in ['y','n','yes','no'] :
            ans=self.getUserInput('[%sconfirm%s] - %s : [%sy/n%s]' % (Color.YELLOW,Color.END,message,Color.CYAN,Color.END))
        return ans.lower() in ['y' , 'yes']

    def getUserInput(self,message) :
        ans=None
        while ans==None or len(ans.lower().strip())==0 :
            ans=raw_input('> %s : ' % (message))
        return ans.strip()

    def markStepSuccess(self,menuitem):
        if 'steps' not in self.data:
            self.data['steps'] = {}

        if type(menuitem) == types.StringType:
            self.data['steps'][menuitem] = True
        else:
            self.data['steps'][menuitem[2]] = True

    def isStepDone(self,menuitem):
        if 'steps' in self.data:
            if type(menuitem) == types.StringType:
                return self.data['steps'].get(menuitem,False)
            else:
                return self.data['steps'].get(menuitem[2],False)
        return False

    def stepRunAll(self,menuitem):
        for item in self.menu[1:]:
            if self.confirm(' %d - %s' % ( item[0], item[1])):
                item[3](item)
            else:
                break;

    def stepConfigureNode(self,menuitem):
        try :
            log.debug('running %s', menuitem[2])
            ip = ''
            if self.data.get('om-ip',None) != None:
                log.info('previously configured om-ip : %s', self.data.get('om-ip'))
            while True:
                ip = self.getUserInput('enter om ip')
                if not self.IP.match(ip):
                    log.warn('not a valid IP')
                else:
                    break
                
            self.data['om-ip'] = ip
            self.markStepSuccess(menuitem)
        except:
            log.error('exception during step %s', menuitem[2])

        
    def stepSystemCheck(self,menuitem):
        try :
            log.debug('running %s', menuitem[2])
            self.markStepSuccess(menuitem)
        except:
            log.error('exception during step %s', menuitem[2])

    def stepDependencyCheck(self,menuitem):
        try :
            log.debug('running %s', menuitem[2])
            self.markStepSuccess(menuitem)
        except:
            log.error('exception during step %s', menuitem[2])

    def stepInstallPackages(self,menuitem):
        try :
            log.debug('running %s', menuitem[2])
            self.markStepSuccess(menuitem)
        except:
            log.error('exception during step %s', menuitem[2])

    def run(self):

        print'%s****************************************************************%s' % ( Color.YELLOW, Color.END)
        print'%s  FDS System installation %s                                      ' % ( Color.BOLDWHITE, Color.END)
        print'%s  Ctrl-D to exit , Ctrl-C to cancel current command %s            ' % ( Color.WHITE, Color.END)
        print'%s****************************************************************%s' % ( Color.YELLOW, Color.END)
        
        while True:
            try:
                print;
                print '>>>> %sInstall Menu%s :' % (Color.BOLDWHITE,Color.END)
                for item in self.menu:
                    print ' [%s%d%s] : %s%s%s' %( Color.BOLDBLUE,item[0],Color.END, Color.GREEN,item[1],Color.END)
                    
                print ''
                while True:
                    step = self.getUserInput("which install step do you want to run?")
                    num = -1
                    try:
                        num = int(step)
                        if num < self.menu[0][0] or num> self.menu[-1][0]:
                            raise Exception()
                        break
                    except:
                        log.error('invalid intall step')
                
                # call the method
                self.menu[num][3](self.menu[num])

            except KeyboardInterrupt:
                print ;
            except EOFError:
                break;

        self.save()
        self.close()

        print;

if __name__ == "__main__":
    fds = Installer()
    fds.run()
            
        



