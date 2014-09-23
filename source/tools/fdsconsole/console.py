#!/usr/bin/env python
import sys
import cmd
import types
import shlex
import shelve
import os
import traceback
import context
import helpers

from helpers import *
from tabulate import tabulate

from contexts.svchelper import ServiceMap
# import needed contexts
from contexts import volume
from contexts import snapshot
from contexts import snapshotpolicy
from contexts import service
from contexts import user

"""
Console exit exception. This is needed to exit cleanly as 
external libraries (argh) are throwing SystemExit Exception on errors.
"""
class ConsoleExit(Exception):
    pass

class FDSConsole(cmd.Cmd):

    def __init__(self,*args):
        cmd.Cmd.__init__(self, *args)
        setupHistoryFile()
        datafile = os.path.join(os.path.expanduser("~"), ".fdsconsole_data")
        self.data = {}
        try :
            self.data = shelve.open(datafile,writeback=True)
        except:
            pass

        self.config = ConfigData(self.data)
            
        self.prompt = 'fds:> '
        self.context = None
        self.previouscontext = None
        self.setupDefaultConfig()
        self.set_root_context(context.RootContext(self.config))
        ServiceMap.init(self.config.getSystem('host'), self.config.getSystem('port'))

    def setupDefaultConfig(self):
        defaults = {
            KEY_ACCESSLEVEL: AccessLevel.USER,
            'host' : '127.0.0.1',
            'port' : 7020
        }
        for key in defaults.keys():
            if None == self.config.getSystem(key):
                #print 'setting default %s' % (key)
                self.config.setSystem(key, defaults[key])

    def get_access_level(self):
        return self.config.getSystem(KEY_ACCESSLEVEL)

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
        self.prompt = ':'.join(promptlist) + ':> '

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
        if line.startswith('?'):
            line = '? ' + line[1:]

        if line.endswith('?'):
            line = line[:-1] + ' ?'

        argv = shlex.split(line)

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
            return ' '.join(argv)

        return line

    def do_refresh(self, line):
        print 'reconnecting to {}:{}'.format(self.config.getSystem('host'), self.config.getSystem('port'))
        ServiceMap.init(self.config.getHost(), self.config.getPort())
        ServiceMap.refresh()

    def help_refresh(self, *args):
        print 'refresh : reconnect to the servers and get node data'

    def complete_refresh(self, *args):
        return []

    def do_accesslevel(self, line):
        argv = shlex.split(line)

        if len(argv) == 0:
            print 'current access level : %s' % (AccessLevel.getName(self.config.getSystem(KEY_ACCESSLEVEL)))
            return

        level = AccessLevel.getLevel(argv[0])

        if level == 0:
            print 'invalid access level : %s' % (argv[0])
            return
        elif level == self.config.getSystem(KEY_ACCESSLEVEL):
            print 'access level is already @ %s' % (AccessLevel.getName(self.config.getSystem(KEY_ACCESSLEVEL))) 
        else:            
            print 'switching access level from [%s] to [%s]' % (AccessLevel.getName(self.config.getSystem(KEY_ACCESSLEVEL)), AccessLevel.getName(level))
            self.config.setSystem(KEY_ACCESSLEVEL, level)

    def help_accesslevel(self, *args):
        print 'usage   : accesslevel [level]'
        print '    -- prints or sets the current access level'
        print '[level] : %s' % (AccessLevel.getLevels())

    def complete_accesslevel(self, text, line, *ignored):
        argv = shlex.split(line)
        if len(argv) > 1 and text != argv[1]:
            return []
        return [c for c in AccessLevel.getLevels() if c.startswith(text)]

    def do_help(self, line):
        argv = shlex.split(line)
        ctx, pos, isctxmatch = self.get_context_for_command(argv)
        if pos == len(argv)-1 and ctx != None and isctxmatch:
            # this means all are sub contexts
            argv = []
        else:
            ctx = self.context
            
        if len(argv) == 0 or argv[0] in ['help']:
            self.print_topics("commands in context" , sorted(ctx.get_method_names(self.config.getSystem(KEY_ACCESSLEVEL))),   15,80)
            self.print_topics("subcontexts : [use cc <context> to switch]",ctx.get_subcontext_names(), 15, 80)
            self.print_topics("globals",self.get_global_commands(), 15, 80)
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

    def do_cc(self, line):
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
            print tabulate(data, headers=['name', 'value'])

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
        print line
        argv = shlex.split(line)
        

    def get_names(self):
        names = [key for key,value in self.context.methods.items() if value <= self.config.getSystem(KEY_ACCESSLEVEL)]
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
        l.extend([item for item, level in ctx.methods.items() if level <= self.config.getSystem(KEY_ACCESSLEVEL) and item.startswith(text)])
        return l


    def get_global_commands(self):
        return [a[3:] for a in cmd.Cmd.get_names(self) if a.startswith('do_')]
        
    def get_context_for_command(self, argv):
        'returns the correct ctx & the pos of match & the type of match'
        ctx = self.context
        pos = -1
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
            return True if ctx.methods[argv[pos]] <= self.config.getSystem(KEY_ACCESSLEVEL) else False
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
                print 'oops!!!! you do not have privileges to run this command'
            else:
                ctx, pos, m = self.get_context_for_command(argv)
                if ctx == None:
                    print 'unable to determine correct context!!!'
                    ctx = self.context
                    pos = 0
                #print 'dispatching : %s' % (argv[pos:])            
                ctx.parser.dispatch(argv[pos:])
        except ConsoleExit:
            raise
        except SystemExit:
            pass
        except:
            #traceback.print_exc()
            print 'cmd: {} exception: {}'.format(line, sys.exc_info()[0])
        return None

    def emptyline(self):
        return ''

    def init(self):
        vol = self.root.add_sub_context(volume.VolumeContext(self.config,'volume'))
        snap = vol.add_sub_context(snapshot.SnapshotContext(self.config,'snapshot'))
        snap.add_sub_context(snapshotpolicy.SnapshotPolicyContext(self.config,'policy'))

        self.root.add_sub_context(service.ServiceContext(self.config,'service'))
        self.root.add_sub_context(user.UserContext(self.config,'user'))

    def run(self, argv = None):
        l =  []
        l += ['============================================']
        l += ['Formation Data Systems Console ...']
        l += ['Copyright 2014 Formation Data Systems, Inc.']
        l += ['>>> NOTE: the current access level : %s' % (AccessLevel.getName(self.config.getSystem(KEY_ACCESSLEVEL)))]
        l += ['============================================']
        l += ['']
        try:
            if argv == None or len(argv) == 0 : 
                l += ['---- interactive mode ----\n']
                self.cmdloop('\n'.join(l))
            else:
                #l += ['---- Single command mode ----\n']
                print '\n'.join(l)
                self.onecmd(self.precmd(' '.join(argv)))
        except (KeyboardInterrupt, ConsoleExit):
            print ''
        self.data.close()

if __name__ == '__main__':
    fdsconsole = FDSConsole()
    fdsconsole.init()
    fdsconsole.run(sys.argv[1:])
    
    
