#!/usr/bin/env python

import shelve
import os
import logging
import re
import readline
import types

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

logging.basicConfig(level=logging.INFO,format="%(levelname)s : %(message)s")
log = logging.getLogger('installer')
log.setLevel(logging.DEBUG)


class Installer:
    def __init__(self):
        self.datafile = '/tmp/.fdsinstall.data'
        self.data = shelve.open(self.datafile,writeback=True)
        self.IP=re.compile('([0-9]{1,3}\.){3}[0-9]{1,3}')
        self.ignoreStepDependency = False
        self.initMenu()

    def initMenu(self):
        self.menu = []
        
        self.menu.append([0,"Run All Steps"              , 'stepRunAll'         ])
        self.menu.append([1,"Run System Check"           , 'stepSystemCheck'    ])
        self.menu.append([2,"Install dependency packages", 'stepDependencyCheck'])
        self.menu.append([3,"Setup Node"                 , 'stepConfigureNode'  ])        
        self.menu.append([4,"Install FDS Packages"       , 'stepInstallFdsPackages'])
        self.menu.append([5,"Toggle Step Dependency check", 'stepToggleStepDependency'])
        
        # set the correct callback functions
        for item in self.menu:
            item.append(getattr(self,item[2]))

    def getMenuByName(self,name):
        if not name.startswith('step'):
            name = 'step'+ name

        for item in self.menu:
            if item[2] == name:
                return item

        return None

    def checkIp(self,ip):
        return 0 == os.system('ping -c 1 %s -W 2 >/dev/null 2>&1' % (ip))

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

    def markStepSuccess(self,menuitem,success=True):
        if 'steps' not in self.data:
            self.data['steps'] = {}

        if type(menuitem) == types.StringType:
            self.data['steps'][menuitem] = success
        else:
            self.data['steps'][menuitem[2]] = success

    def isStepDone(self,menuitem):
        if 'steps' in self.data:
            if type(menuitem) == types.StringType:
                return self.data['steps'].get(menuitem,False)
            else:
                return self.data['steps'].get(menuitem[2],False)
        return False

    def stepRunAll(self,menuitem):
        for item in self.menu[1:5]:
            if self.confirm(' %d - %s' % ( item[0], item[1])):
                item[3](item)
            else:
                break;

    def dependsOnSteps(self,steps) :
        if type(steps) == types.StringType:
            steps=[steps]

        success=True
        for step in steps:
            if not self.isStepDone(step):
                menuitem = self.getMenuByName(step)
                log.warn("please finish step [%d - %s] before continuing", menuitem[0], menuitem[1])
                success=False

        if not success and not self.ignoreStepDependency:
            raise Exception('prior steps not completed')

    def stepToggleStepDependency(self,menuitem):
        if not self.ignoreStepDependency:
            if not self.confirm("are you sure you want to ignore step dependencies"):
                return

        self.ignoreStepDependency = not self.ignoreStepDependency
        log.warn('ignore step dependency is now [%s]' ,self.ignoreStepDependency)

    def stepConfigureNode(self,menuitem):
        try :
            self.dependsOnSteps('stepDependencyCheck')
            if self.isStepDone(menuitem):
                log.info("NOTE: This step has been completed previously")

            log.debug('running %s', menuitem[2])
            ret = os.system("python ./disk_type.py -f")
            success=True
            if 0 != ret:
                success=False
                log.error("install disk partition/format did not complete successfully")
            
            self.markStepSuccess(menuitem,success)
        except:
            self.markStepSuccess(menuitem,False)
            log.error('exception during step %s', menuitem[2])
        
    def stepSystemCheck(self,menuitem):
        try :
            if self.isStepDone(menuitem):
                log.info("NOTE: This step has been completed previously")
            log.debug('running %s', menuitem[2])
            success=True
            ret = os.system("python ./system_pre_check.py")
            if 0 != ret:
                success=False
                log.error("system precheck did not complete successfully")

            self.markStepSuccess(menuitem,success)
        except:
            self.markStepSuccess(menuitem,False)
            log.error('exception during step %s', menuitem[2])

    def stepDependencyCheck(self,menuitem):
        try :
            self.dependsOnSteps('stepSystemCheck')
            log.debug('running %s', menuitem[2])
            success=True
            for option in ['prereq', 'basedebs','pythonpkgs'] :
                ret = os.system("./setup-packages.sh %s" % (option))
                if 0 != ret :
                    success=False
                    log.error("install [%s] did not complete successfully", option)
                    if not self.confirm("do you want to continue further?"):
                        break;
            self.markStepSuccess(menuitem,success)
        except:
            self.markStepSuccess(menuitem,False)
            log.error('exception during step %s', menuitem[2])

    def stepInstallFdsPackages(self,menuitem):
        try :
            self.dependsOnSteps('stepConfigureNode')
            if self.isStepDone(menuitem):
                log.info("NOTE: This step has been completed previously")
            log.debug('running %s', menuitem[2])
            success=True
            for option in ['fds-base','fds-om','fds-pm','fds-am','fds-sm','fds-cli','fds-dm'] :
                ret = os.system("./setup-packages.sh %s" % (option))
                if 0 != ret :
                    success=False
                    log.error("install [%s] did not complete successfully", option)
                    if not self.confirm("do you want to continue further?"):
                        break;
            self.markStepSuccess(menuitem,success)
        except:
            self.markStepSuccess(menuitem,False)
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
                    step = self.getUserInput("which install step do you want to run (Ctrl-D to exit)?")
                    num = -1
                    try:
                        num = int(step)
                        if num < self.menu[0][0] or num> self.menu[-1][0]:
                            raise Exception()
                        break
                    except:
                        log.error('invalid install step')
                
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
            
        



