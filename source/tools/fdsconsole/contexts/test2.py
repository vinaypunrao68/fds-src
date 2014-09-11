from  fdsconsole.decorators import *
from  fdsconsole.context import Context
import inspect

class Test2(Context):

    def get_context_name(self):
        return "test2"

    @clicmd
    def heaven(self):
        print inspect.stack()[0][3]

    @cliadmincmd
    def devil(self):
        print inspect.stack()[0][3]
