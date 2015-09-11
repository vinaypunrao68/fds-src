import os
import sys
import cmd
import shlex
import pipes
from argparse import ArgumentParser
import pkgutil
from utils.fds_cli_configuration_manager import FdsCliConfigurationManager
from services.fds_auth import FdsAuth
from services.fds_auth_error import FdsAuthError
from requests.exceptions import ConnectionError

class FDSShell( cmd.Cmd ):
    '''
    Created on Mar 30, 2015
    
    This is the main wrapper for the FDS CLI tool.  It's main purpose is to 
    obtain user authorization, load the dynamic modules and setup the 
    parsers.
    
    All actual work will take place in the "plugins" or the "services" that
    are added to the server, this simply manages the state and configures
    argparse
    
    @author: nate bever
    '''

    def __init__(self, session, *args ):
        '''
        initialize the CLI session
        '''
        
        cmd.Cmd.__init__(self, *args)
        
        val = FdsCliConfigurationManager().get_value(FdsCliConfigurationManager.TOGGLES, FdsCliConfigurationManager.CMD_HISTORY)
        
        if val == "true" or val is True or val == "True":
            setupHistoryFile()

        self.plugins = []
        self.__session = session
        
        self.prompt ='fds> '

        self.loadmodules()
        

    def formatClassName(self, name):
        '''
        This makes assumptions about the name of the class relative to the name of the 
        plugin.  It basically deletes all "_" and capitalizes each word so that
        my_cool_plugin is expected to declare class MyCoolPlugin
        '''        
        words = name.split( '_' )
        formattedName = ''
        
        for word in words:
            formattedName = formattedName + word.capitalize()
        
        return formattedName 
        

    def loadmodules(self):
        '''
        This searches the plugins directory (relative to the location of this file)
        and will load all the modules it find there, adding their parsing arguments
        to the argparse setup
        '''
        self.parser = ArgumentParser( add_help=True)
        
        self.subParsers = self.parser.add_subparsers( help="Command suite description" )

        mydir = os.path.dirname( os.path.abspath( __file__ ) )
        modules = pkgutil.iter_modules([os.path.join( mydir, "plugins" )] )
        sys.path.append(mydir)
        
        for loader, mod_name, ispkg in modules:
            
            if ( mod_name == "abstract_plugin" ):
                continue
            
            loadedModule = __import__( "plugins." + mod_name, globals=globals(), fromlist=[mod_name] )

            clazzName = self.formatClassName( mod_name )
            clazz = getattr( loadedModule, clazzName )
            clazz = clazz()
            self.plugins.append( clazz )
            
            clazz.build_parser( self.subParsers, self.__session )
            

    def default(self, line):
        '''
        Default method that gets called when no 'do_*' method
        is found for the command prompt sent in
        '''        
        
        try:
            
            if not self.__session.is_authenticated():
                try:
                    self.__session.login()
                    self.loadmodules()
                    print("Connected to: {}\n".format(self.__session.get_hostname()))
        
                except FdsAuthError as f:
                    print(str(f.error_code) + ":" + f.message)
                    self.__session.logout()
                    return
                except Exception as ex:
                    print("Unknown error occurred.")
                    self.__session.logout()
                    return
            
            argList = shlex.split( line )
            pArgs = self.parser.parse_args( argList )
            pArgs.func( vars( pArgs ) )
        
        # a connection error occurs later
        except ConnectionError:
            print("Lost connection to OM.  Please verify that it is up and responsive.")
            self.__session.logout()
            return
            
        # A system exit gets raised from the argparse stuff when you ask for help.  Stop it.    
        except SystemExit:
            return
       

    def run( self, argv ):
        '''
        Called from main to actually start the CLI tool running
        
        By default Cmd will try to run a do_* argv method if one exists.
        If not, it will call into the loop which ends up in the "default" function
        '''         
        
        # If there are no arguments we will execute as an interactive shell
        if argv is None or len( argv ) == 0:
            self.cmdloop( '\n' )
        # Otherwise just run this one command
        else:
            strCmd = ' '.join( map( pipes.quote, argv ) )
            self.onecmd( strCmd );
        

    def exitCli(self, *args):
        '''
        Just one method to exit the program so we can link a few commands to it
        '''
        print('Bye!')
        sys.exit()
        

    def do_exit(self, *args):
        '''
        Handles the command 'exit' magically.
        '''        
        self.exitCli( *args )
        

    def do_quit(self, *args):
        '''
        Handles the command 'quit' magically
        '''        
        self.exitCli( *args )
        

    def do_bye(self, *args):
        '''
        Handles the command 'bye' magically
        '''
        self.exitCli( *args )
        


def setupHistoryFile():
    '''
    stores and retrieves the command history specific to the user
    '''
         
    import readline
    histfile = os.path.join(os.path.expanduser("~"), ".fdsconsole_history")
    readline.set_history_length(20)
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
    
    shell = FDSShell(auth, cmdargs)

    # now we check argv[0] to see if its a shortcut scripts or not    
    if ( len( cmdargs ) > 0 ):

        for plugin in shell.plugins:
            detectMethod = getattr( plugin, "detect_shortcut", None )
            
            # the plugin does not support shortcut argv[0] stuff
            if ( detectMethod is None or not callable( plugin.detect_shortcut ) ):
                continue
            
            tempArgs = plugin.detect_shortcut( cmdargs )
                
            # we got a new argument set
            if ( tempArgs is not None ):
                cmdargs = tempArgs
                break
        # end of for loop
        
    # now actually run the command
    shell.run( cmdargs ) 
