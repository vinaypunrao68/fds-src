import cmd
"""
class FdsCliCtx(Cmd):
    def do_stats(self):
        pass

class DomainCliCtx(Cmd):
    def do_addnode(self, arg):
        print 'addnode {}'.format(arg)
        pass

    def do_removenode(self):
        pass
    
class NodeCliCtx(FdsCliCtx):
    pass

class SvcCliCtx(FdsCliCtx):
    pass

class SMCliCtx(SvcCliCtx):
    def do_stats(self):
        pass
"""

class MyShell(cmd.Cmd):
    "Illustrate the base class method use."
    
    def cmdloop(self, intro=None):
        print 'cmdloop(%s)' % intro
        return cmd.Cmd.cmdloop(self, intro)
    
    def preloop(self):
        print 'preloop()'
    
    def postloop(self):
        print 'postloop()'
        
    def parseline(self, line):
        print 'parseline(%s) =>' % line,
        ret = cmd.Cmd.parseline(self, line)
        print ret
        return ret
    
    def onecmd(self, s):
        print 'onecmd(%s) =>' % s
        ret = cmd.Cmd.onecmd(self, s)
        print ret
        return ret

    def emptyline(self):
        print 'emptyline()'
        return cmd.Cmd.emptyline(self)
    
    def default(self, line):
        print 'default(%s)' % line
        return cmd.Cmd.default(self, line)
    
    def precmd(self, line):
        print 'precmd(%s)' % line
        return cmd.Cmd.precmd(self, line)
    
    def postcmd(self, stop, line):
        print 'postcmd(%s, %s)' % (stop, line)
        return cmd.Cmd.postcmd(self, stop, line)
    
    def do_greet(self, line):
        print 'hello,', line

    def do_EOF(self, line):
        "Exit"
        return True

if __name__ == '__main__':
    MyShell().cmdloop()
