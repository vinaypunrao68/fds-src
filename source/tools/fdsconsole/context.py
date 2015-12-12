import argh
import inspect
import helpers
import argparse

class Context:
    
    def __init__(self, config=None, name=None):
        self.config = config
        self.name   = name
    '''
    Base class for all contexts
    NOTE:: All contexts SHOULD override get_context_name
    '''
    def get_context_name(self):
        if self.name == None:
            raise Exception("context name not configured")
        return self.name

    def before_entry(self):
        '''
        called when the console is about to enter into the context.
        Should return True to successfully proceed.
        '''
        return True

    def before_exit(self):
        '''
        called when the console is about to exit out of the context.
        Should return True to successfully proceed.
        '''
        return True

class ContextInfo:
    '''
    Basic wrapper structure used by the console.
    NOT to be used in other places.
    '''
    def __init__(self, ctx):
        self.parent = None
        self.context  = ctx
        self.subcontexts = {}
        self.methods = {}
        self.parser = argh.ArghParser(prog='',add_help=False)
        funclist=[]
        #suppress
        for funcinfo in inspect.getmembers(ctx, predicate=inspect.ismethod):
            func = getattr(ctx, funcinfo[0])
            try:
                #print 'processing : %s' % (func.__name__)
                if hasattr(func, helpers.ATTR_CLICMD):
                    funclist.append(func)
                    cleanname = func.__name__.replace('_','-')
                    self.methods[cleanname] = getattr(func, helpers.ATTR_CLICMD)
            except:
                pass
        if len(funclist) > 0:
            self.parser.add_commands(funclist)
            # clean up the display of help
            suppressHelp = True
            if suppressHelp:
                for subaction in self.parser._subparsers._actions:
                    for funcparser in subaction.choices.values():
                        for action in funcparser._get_optional_actions():
                            if action.dest == 'help':
                                action.help = argparse.SUPPRESS
        else:
            #print "no functions found in this context : %s" % (self.context.get_context_name()) 
            pass

    def get_help(self, level):
        helplist=[]
        ctxName = self.context.name + ' ' if self.context.name != None else ''
        if len(self.methods) > 0:
            for cmd, cmdparser in self.parser._subparsers._actions[0].choices.iteritems():
                cleanname = cmd.replace('_','-')
                if self.methods[cleanname] > level:
                    continue
                #print cmd, cmdparser.description
                desc=''
                if cmdparser.description != None:
                    desclist = cmdparser.description.strip().split('\n')
                    if len(desclist)>0:
                        desc = desclist[0]
                helplist.append(('{}{}'.format(ctxName,cmd) , desc))

        for ctx in self.subcontexts.values():
            helplist.extend([('{}{}'.format(ctxName,h[0]), h[1]) for h in ctx.get_help(level)])

        return helplist

    def add_sub_context(self, ctx):
        if not isinstance(ctx, ContextInfo):
            ctx = ContextInfo(ctx)
        ctx.parent = self
        self.subcontexts[ctx.context.get_context_name()] = ctx
        return ctx

    def get_subcontext_names(self):
        return self.subcontexts.keys()

    def get_method_names(self, level = helpers.AccessLevel.ADMIN):
        return [item for item, value in self.methods.items() if value <= level]

class RootContext(Context):

    def __init(self, *args):
        Context.__init__(self, *args)
    '''
    This is the first context ..
    Currently it is a dummy.. Later all shared functionality will be here.
    '''
    def get_context_name(self):
        return "fds"
