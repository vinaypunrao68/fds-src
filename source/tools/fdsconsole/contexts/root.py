from svchelper import *
from svc_api.ttypes import *
from common.ttypes import *
from platformservice import *
import FdspUtils
import tabulate

class RootContext(Context):

    def __init(self, *args):
        Context.__init__(self, *args)
    '''
    This is the first context ..
    Currently it is a dummy.. Later all shared functionality will be here.
    '''
    def get_context_name(self):
        if self.config.isDebugTool():
            return "fds.debug"
        else:
            return "fds"

    @clidebugcmd
    @arg('--volume', help= "volume id", default=None, type=long)
    @arg('--token', help= "token id", default=None, type=long)
    @arg('--disk', help= "disk id", default=None, type=long)
    def expungedb(self,volume=None, token=None, disk=None):
        import sqlite3
        filename = "{}/user-repo/liveobj.db".format(self.config.getFdsRoot())

        if not os.path.exists(filename):
            print 'db does not exist :', filename
            return

        conn = sqlite3.connect(filename)

        cols = []
        for row in conn.execute("PRAGMA table_info('tokentbl')"):
            cols.append(row[1])

        data=[]
        query = "select * from tokentbl where timestamp>0 " 

        if token!= None:
            query += " and smtoken={}".format(token)

        if disk!= None:
            query += " and diskid={}".format(disk)
        
        query += " order by smtoken,diskid"
        for row in conn.execute(query):
            data.append(row)

        print tabulate.tabulate(data, headers=cols)
        print 'num rows : {}'.format(len(data))


        cols = []
        for row in conn.execute("PRAGMA table_info('liveobjectstbl')"):
            cols.append(row[1])

        data=[]
        query = "select * from liveobjectstbl where length(filename)>0 " 
        if volume != None:
            query += " and volid={}".format(volume)

        if token!= None:
            query += " and smtoken={}".format(token)
        
        query += " order by smtoken,volid"
        for row in conn.execute(query):
            data.append(row)

        print tabulate.tabulate(data, headers=cols)
        print 'num rows : {}'.format(len(data))
