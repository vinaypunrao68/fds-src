from  fdsconsole.decorators import *
from  fdsconsole.context import Context
import inspect

class Test1(Context):

    def get_context_name(self):
        return "test1"

    @clicmd
    def hell(self):
        print inspect.stack()[0][3]

    @clidebugcmd
    def god(self):
        print inspect.stack()[0][3]
