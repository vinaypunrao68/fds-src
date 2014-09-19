import argh
import inspect
import helpers

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
        else:
            #print "no functions found in this context : %s" % (self.context.get_context_name()) 
            pass

    def add_sub_context(self, ctx):
        if isinstance(ctx, Context):
            ctx = ContextInfo(ctx)
        ctx.parent = self
        self.subcontexts[ctx.context.get_context_name()] = ctx
        return ctx

    def get_subcontext_names(self):
        return self.subcontexts.keys()

    def get_method_names(self, level = helpers.AccessLevel.DEBUG):
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
