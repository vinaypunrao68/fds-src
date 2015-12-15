#!/usr/bin/env python
try:
    import readline
except ImportError:
    pass;

import shlex
import cmd
import shelve
import os
import traceback
import context
import helpers
import pipes
import time
from helpers import *
from tabulate import tabulate
import sys
from contexts.svchelper import ServiceMap
# import needed contexts
from contexts import domain
from contexts import volume
from contexts import snapshot
from contexts import snapshotpolicy
from contexts import QoSPolicy
from contexts import service
from contexts import user
from contexts import tenant
from contexts import scavenger
from contexts import ScavengerPolicy
from contexts import SMDebug
from contexts import DMDebug

import warnings
# this is soooo bad
warnings.simplefilter('ignore')


"""
Console exit exception. This is needed to exit cleanly as
external libraries (argh) are throwing SystemExit Exception on errors.
"""
class ConsoleExit(Exception):
    pass

def checkFDSNode():
    if os.path.isfile('/fds/etc/platform.conf'): return True

class FDSConsole(cmd.Cmd):

    def __init__(self,fInit, debugTool, *args):
        cmd.Cmd.__init__(self, *args)
        setupHistoryFile(debugTool)
        if debugTool:
            datafile = os.path.join(os.path.expanduser("~"), ".debugtool_data")
        else:
            datafile = os.path.join(os.path.expanduser("~"), ".fdsconsole_data")
        self.data = {}
        self.root = None
        self.recordFile = None
        try :
            self.data = shelve.open(datafile,writeback=True)
        except:
            pass
        self.data['debugTool'] = debugTool
        self.debugTool = debugTool
        self.config = ConfigData(self.data)
        self.myprompt = 'fds'
        if self.debugTool:
            self.myprompt = 'fds.debug'
        self.setprompt(self.myprompt)
        self.context = None
        self.previouscontext = None
        ServiceMap.config = self.config
        if fInit:
            self.config.init()
            self.set_root_context(context.RootContext(self.config))

    def get_access_level(self):
        if self.debugTool:
            return AccessLevel.DEBUG
        else:
            return AccessLevel.ADMIN

    def set_root_context(self, ctx):
        if isinstance(ctx, context.Context):
            ctx = context.ContextInfo(ctx)
        self.root = ctx
        self.set_context(ctx)
        return ctx

    def set_context(self, ctx):
        '''
        sets a new context if it passes checks
        '''
        if self.context and not self.context.context.before_exit():
            print 'unable to exit from current context'
            return False

        self.previouscontext = self.context

        if not ctx.context.before_entry():
            print 'unable to enter into the new context'
            return False

        self.context = ctx
        promptlist=[]
        while ctx != None:
            promptlist.append(ctx.context.get_context_name())
            ctx = ctx.parent

        promptlist.reverse()
        self.setprompt(':'.join(promptlist))

    def setprompt(self, p=None):
        if p == None:
            p = self.myprompt
        else:
            self.myprompt = p
        self.prompt = '[' + time.strftime('%H:%M:%S') + '] ' + p + ':> '

    def postcmd(self, stop, line):
        self.setprompt()


    def precmd(self, line):
        '''
        before a line is processed, , cleanup the line...
        eg :
          ?cmd -> ? cmd
          cmd? -> cmd ?
          cmd -h -> help cmd
          ..  -> cc ..
          -   -> cc -
        '''

        if self.recordFile:
            argv = shlex.split(line)
            if not (len(argv) > 0 and argv[0].lower() == 'record'):
                if len(line.strip()) > 0 :
                    self.recordFile.write(line)
                    self.recordFile.write('\n')

        if line.startswith('?'):
            line = '? ' + line[1:]

        if line.endswith('?'):
            line = line[:-1] + ' ?'

        argv = shlex.split(line)

        # remove comments
        for n in xrange(0,len(argv)):
            if argv[n].startswith('#'):
                del argv[n:]
                break

        if len(argv) == 1:
            if argv[0] == '..':
                return 'cc ..'
            elif argv[0] == '-':
                return 'cc -'

        if len(argv) > 0:
            if argv[0] in ['?','-h','--help']:
                argv[0] = 'help'

            if argv[-1] in ['?','-h','--help']:
                del argv[-1]
                argv.insert(0,'help')

        ctx, pos, isctx = self.get_context_for_command(argv)
        if pos == len(argv)-1 and isctx and len(argv) > 0:
            argv.insert(0,'help')

        #print ' '.join (map(pipes.quote, argv))
        return ' '.join (map(pipes.quote, argv))

    def do_refresh(self, line):
        print 'reconnecting to {}:{}'.format(self.config.getSystem('host'), self.config.getSystem('port'))
        ServiceMap.init(self.config.getHost(), self.config.getPort())
        ServiceMap.refresh()

    def help_refresh(self, *args):
        print 'refresh : reconnect to the servers and get node data'

    def complete_refresh(self, *args):
        return []

    def do_help(self, line):
        argv = shlex.split(line)
        ctx, pos, isctxmatch = self.get_context_for_command(argv)
        if pos == len(argv)-1 and ctx != None and isctxmatch:
            # this means all are sub contexts
            argv = []
        else:
            ctx = self.context
        #print self.get_access_level()
        #print AccessLevel.getName(self.get_access_level())
        if len(argv) == 0 or argv[0] in ['help']:

            helplist=sorted(ctx.get_help(self.get_access_level()), key=lambda h: '{}{}'.format(h[0],h[1]))
            print tabulate(helplist,headers= ['command', 'description'],
                            tablefmt=self.config.getTableFormat())
            if ctx == self.root:
                self.print_topics("\nother commands" , sorted(self.get_global_commands()),   15,80)
        else:
            if line in self.get_global_commands():
                if hasattr(self,'help_' + line):
                    getattr(self,'help_' + line)()
            else:
                self.default(line + ' -h')

    def help_cc(self, *ignored):
        print 'cc : change context'
        print 'usage: cc [contextname]'

    def complete_help(self, text, line , *args):
        argv = shlex.split(line)
        if len(argv) > 0 and argv[0] in ['?', 'help']:
            del argv[0]

        if len(argv) == 0 or len(argv) == 1 and argv[0] == text:
            return self.completenames(text, line, *args)
        else:
            return self.completedefault(text, argv, *args)

    def do_run(self, line):
        argv = shlex.split(line)

        if len(argv) != 1:
            print 'filename needed..'
            return

        filename = os.path.expandvars(os.path.expanduser(argv[0]))
        if not os.path.isfile(filename):
            print 'unable to locate file :', filename
            return

        lines = [l.strip() for l in open(filename)]

        print '{} commands will be executed'.format(len(lines))

        for line in lines:
            print 'cmd : {}'.format(line)
            self.onecmd(line)

    def help_run(self):
        print 'run <filename> : run the commands from a file'

    def complete_run(self, line):
        return []

    def do_record(self, line):
        argv = shlex.split(line)

        if self.recordFile != None:
            if len(argv) != 1:
                print 'recording in progress .. [type record stop] to stop the recording.'
            elif argv[0].lower() == 'stop':
                self.recordFile.close()
                self.recordFile = None
                print 'recording stopped .'
            else:
                print 'unknown command'
            return

        if len(argv) != 1:
            print 'filename needed..'
            return

        if argv[0].lower() == 'stop':
            print 'recording has not yet started .'
            return

        filename = os.path.expandvars(os.path.expanduser(argv[0]))
        try :
            self.recordFile = open(filename,'a')
            print 'recording started'
        except Exception as e:
            print e

    def help_record(self):
        print 'usage: record [stop|filename]'
        print '  -- record filename : will start recording the commands into the file'
        print '  -- record stop : will stop the current recording'

    def complete_record(self, text, line, *args):
        argv = shlex.split(line)
        if len(argv) > 1:
            return [item for item in ['stop'] if item.startswith(argv[1])]
        return [item for item in ['stop'] if item.startswith(text)]

    def _do_cc(self, line):
        ctxName = line
        if len(ctxName) == 0:
            return self.help_cc()

        if ctxName == "..":
            if self.context.parent:
                self.set_context(self.context.parent)
                return
            else:
                print 'no higher context'
                return

        if ctxName == '-':
            if self.previouscontext:
                self.set_context(self.previouscontext)
                return
            else:
                print 'no previous context'
                return

        if ctxName in self.context.subcontexts.keys():
            self.set_context(self.context.subcontexts[ctxName])
            return

        print 'invalid context : %s' % (ctxName)

    def complete_cc(self, text, *ignored):
        return [c for c in self.context.get_subcontext_names() if c.startswith(text)]

    def do_set(self, line):
        argv = shlex.split(line)
        if len(argv) == 0:
            data = []
            # print the current values
            for key,value in self.data[helpers.KEY_SYSTEM].items():
                if key not in helpers.PROTECTED_KEYS:
                    data.append([key, str(value)])
            #print data
            print tabulate(data, headers=['name', 'value'], tablefmt=self.config.getTableFormat())

            return

        argv[0] = argv[0].lower()
        if len(argv) == 1:
            # print the value of this key
            print '%10s   =  %5s' % (argv[0], self.config.getSystem(argv[0]))
            return

        if argv[0] in helpers.PROTECTED_KEYS:
            print 'cannot modify a protected system key : %s' % (argv[0])
            return

        self.config.setSystem(argv[0], argv[1])

    def help_set(self,*args) :
        print 'usage: set [key] [value]'
        print '-- sets a value to a key'
        print '-- to see current values, just type set'
        print '-- to see current value of key, type set <key>'

    def complete_set(self, text, line, *args):
        argv = shlex.split(line)
        cmd = ''
        if len(argv) == 2:
            cmd = argv[1]
        returnList = []
        for key in self.data[helpers.KEY_SYSTEM].keys():
            if key not in helpers.PROTECTED_KEYS and key.startswith(cmd) and key != cmd:
                returnList.append(key)

        return returnList

    def do_exit(self, *args):
        raise ConsoleExit()

    def help_exit(self, *args):
        print 'exit the fds console'

    def get_names(self):
        names = [key for key,value in self.context.methods.items() if value <= self.get_access_level()]
        names.extend(self.context.subcontexts.keys())
        return names

    def completenames(self, text, *ignored):
        #print "[%s] -- %s" %(inspect.stack()[0][3], text)
        return  [c for c in self.get_names() + self.get_global_commands() if c.startswith(text)]

    def completedefault(self, text, line, begidx, endidx):
        if type(line) == types.ListType:
            argv = line
        else:
            argv=shlex.split(line)

        ctx = self.context
        #print "[%s] -- %s : %s : %d, %d" %(inspect.stack()[0][3], text, line, begidx, endidx)
        for name in argv:
            if name in ctx.subcontexts:
                ctx = ctx.subcontexts[name]
            elif name in ctx.methods:
                return []
                #return [item for item, level in ctx.methods.items() if level <= self.config.getSystem(KEY_ACCESSLEVEL) and item.startswith(name)]
            else:
                break

        # hack
        #if len(argv)>0 and argv[-1].find('-') != -1 and not line.endswith(' '):
        #    text = argv[-1]

        #print 'search text is %s' %(text)

        l = [item for item in  ctx.subcontexts.keys() if item.startswith(text)]
        l.extend([item for item, level in ctx.methods.items() if level <= self.get_access_level() and item.startswith(text)])
        return l


    def get_global_commands(self):
        return [a[3:] for a in cmd.Cmd.get_names(self) if a.startswith('do_')]

    def get_context_for_command(self, argv):
        'returns the correct ctx & the pos of match & the type of match'
        ctx = self.context
        pos = -1
        if ctx == None:
            return (None, -1, None)

        for name in argv:
            pos += 1
            # check the current functions
            if name in ctx.methods.keys():
                # False means function match
                return (ctx, pos, False)
            elif name in ctx.subcontexts.keys():
                #it is a subcontext name
                ctx = ctx.subcontexts[name]
            else:
                return (None, pos, None)
        # True means context match
        return (ctx, pos, True)

    def has_access(self, argv):
        'check if the current access level allows this function'
        ctx, pos, m = self.get_context_for_command(argv)
        if ctx:
            return True if ctx.methods[argv[pos]] <= self.get_access_level() else False
        else:
            return None

    def default(self, line):
        try:
            argv=shlex.split(line)

            if len(argv) == 1:
                # ^-D : Ctrl-D
                if line == 'EOF':
                    raise ConsoleExit()
            else:
                if argv[-1] == '?':
                    argv[-1] = '-h'

            if self.has_access(argv) == False:
                print 'command not available'
                return None
            ctx, pos, isctx = self.get_context_for_command(argv)
            if ctx == None:
                print '[error] : unknown command :',line
                return None

            #print 'dispatching:{}:{}:{}'.format(argv[pos:], isctx, ctx)
            ctx.parser.dispatch(argv[pos:])
        except ConsoleExit:
            raise
        except SystemExit:
            pass
        except:
            #traceback.print_exc()
            #print sys.exc_info()
            print 'cmd: {} exception: {}'.format(line, sys.exc_info()[1])
        return None

    def emptyline(self):
        return ''

    def init(self):
        self.root.add_sub_context(domain.DomainContext(self.config,'domain'))
        vol = self.root.add_sub_context(volume.VolumeContext(self.config,'volume'))
        snap = vol.add_sub_context(snapshot.SnapshotContext(self.config,'snapshot'))
        snap.add_sub_context(snapshotpolicy.SnapshotPolicyContext(self.config,'policy'))
        self.root.add_sub_context(QoSPolicy.QoSPolicyContext(self.config,'qospolicy'))

        self.root.add_sub_context(service.ServiceContext(self.config,'service'))
        self.root.add_sub_context(tenant.TenantContext(self.config,'tenant'))
        self.root.add_sub_context(user.UserContext(self.config,'user'))
        self.root.add_sub_context(SMDebug.SMDebugContext(self.config, 'smdebug'))
        self.root.add_sub_context(DMDebug.DMDebugContext(self.config, 'dmdebug'))
        scav = self.root.add_sub_context(scavenger.ScavengerContext(self.config,'gc'))
        scav.add_sub_context(ScavengerPolicy.ScavengerPolicyContext(self.config, 'policy'))
        scav.add_sub_context(scavenger.ScrubberContext(self.config, 'scrubber'))

    def run(self, argv = None):
        l =  []
        l += ['============================================']
        l += ['Copyright 2014 Formation Data Systems, Inc.']
        if self.debugTool:
            l+= ['>>> DebugTool - Works only on fds nodes !!']
        l += ['NOTE: Ctrl-D , Ctrl-C to exit']
        l += ['============================================']
        l += ['']

        if self.debugTool and not checkFDSNode():
            print 'Debug Tool should be used only on fds nodes. This node does NOT seem to be one!!'
            sys.exit(1)

        try:
            if argv == None or len(argv) == 0 :
                #l += ['---- interactive mode ----\n']
                self.cmdloop('\n'.join(l))
            else:
                #l += ['---- Single command mode ----\n']
                self.onecmd(self.precmd(' '.join(argv)))
        except (KeyboardInterrupt, ConsoleExit):
            print ''

        if self.config.hasPlatformClient():
            self.config.getPlatform().stop_server()

        if self.recordFile:
            self.recordFile.close()

        self.data.close()

if __name__ == '__main__':
    cmdargs=sys.argv[1:]
    fInit = not (len(cmdargs) > 0 and cmdargs[0] == 'set')
    fdsconsole = FDSConsole(fInit, True)
    if fInit:
        fdsconsole.init()
    fdsconsole.run(cmdargs)
