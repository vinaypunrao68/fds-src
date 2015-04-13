'''
Created on Mar 30, 2015

@author: nate bever
'''
import os
import sys
import cmd
import shlex
import pipes
from argparse import ArgumentParser
import pkgutil
from services.fds_auth import FdsAuth

class FDSShell( cmd.Cmd ):
   
    '''
    initialize the CLI session
    '''
    def __init__(self, session, *args ):
        
        cmd.Cmd.__init__(self, *args)
        setupHistoryFile()
        self.plugins = []
        self.__session = session
        
        self.prompt ='fds> '
        self.parser = ArgumentParser( add_help=True)
        
        self.subParsers = self.parser.add_subparsers( help="Sub-commands" )
        self.loadmodules()
        
    def formatClassName(self, name):
        words = name.split( '_' )
        formattedName = ''
        
        for word in words:
            formattedName = formattedName + word.capitalize()
        
        return formattedName 
        
    def loadmodules(self):
        mydir = os.path.dirname( os.path.abspath( __file__ ) )
        modules = pkgutil.iter_modules([os.path.join( mydir, "plugins" )] )
        
        for loader, mod_name, ispkg in modules:
            
            if ( mod_name == "abstract_plugin" ):
                continue
            
            loadedModule = __import__( "fds.plugins." + mod_name, fromlist=[mod_name] )

            clazzName = self.formatClassName( mod_name )
            clazz = getattr( loadedModule, clazzName )
            clazz = clazz()
            self.plugins.append( clazz )
            
            clazz.build_parser( self.subParsers, self.__session )
            
    '''
    Default method that gets called when no 'do_*' method
    is found for the command prompt sent in
    '''
    def default(self, line):
        
        try:
            argList = shlex.split( line )
            pArgs = self.parser.parse_args( argList )
            pArgs.func( vars( pArgs ) )
            
        # A system exit gets raised from the argparse stuff when you ask for help.  Stop it.    
        except SystemExit:
            return
       
    '''
    Called from main to actually start the CLI tool running
    
    By default Cmd will try to run a do_* argv method if one exists.
    If not, it will call into the loop which ends up in the "default" function
    ''' 
    def run( self, argv ):
        
        # If there are no arguments we will execute as an interactive shell
        if argv == None or len( argv ) == 0:
            self.cmdloop( '\n' )
        # Otherwise just run this one command
        else:
            strCmd = ' '.join( map( pipes.quote, argv ) )
            self.onecmd( strCmd );
        
    '''
    Just one method to exit the program so we can link a few commands to it
    '''
    def exitCli(self, *args):
        print 'Bye!'
        sys.exit()
        
    '''
    Handles the command 'exit' magically.
    '''
    def do_exit(self, *args):
        self.exitCli( *args )
        
    '''
    Handles the command 'quit' magically
    '''
    def do_quit(self, *args):
        self.exitCli( *args )
        

'''
stores and retrieves the command history specific to the user
'''

def setupHistoryFile():
    import readline
    histfile = os.path.join(os.path.expanduser("~"), ".fdsconsole_history")
    try:
        readline.read_history_file(histfile)
    except IOError:
        pass
    import atexit
    atexit.register(readline.write_history_file, histfile)

'''
The main entry point to the application.  
Instantiates the CLI object and passes that args in
'''
if __name__ == '__main__':
        
    cmdargs=sys.argv[1:]

    auth = FdsAuth()
    token = auth.login()
    
    #login failures will return empty tokens
    if ( token == None ):
        print 'Authentication failed.'
        sys.exit( 1 )
    
    shell = FDSShell(auth, cmdargs)

    # now we check argv[0] to see if its a shortcut scripts or not    
    if ( len( cmdargs ) > 0 ):

        for plugin in shell.plugins:
            detectMethod = getattr( plugin, "detect_shortcut", None )
            
            # the plugin does not support shortcut argv[0] stuff
            if ( detectMethod == None or not callable( plugin.detect_shortcut ) ):
                continue
            
            tempArgs = plugin.detect_shortcut( cmdargs )
                
            # we got a new argument set
            if ( tempArgs != None ):
                cmdargs = tempArgs
                break
        # end of for loop
        
    # now actually run the command
    shell.run( cmdargs ) 