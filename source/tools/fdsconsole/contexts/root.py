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
        'print expunge db info'
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

    @clidebugcmd
    @arg('volume', help= "volume id/name", type=str, default=None, nargs='?')
    def timeline(self, volume):
        'dump timeline db info'
        import sqlite3
        filename = "{}/sys-repo/dm-names/timeline.db".format(self.config.getFdsRoot())

        if not os.path.exists(filename):
            print 'db does not exist :', filename
            return

        volstr = ''
        if volume != None:
            volid = self.config.getVolumeApi().getVolumeId(volume)
            volstr = ' where volid = {}'.format(volid)

        conn = sqlite3.connect(filename)
        snapshotcols = []
        for row in conn.execute("PRAGMA table_info('snapshottbl')"):
            snapshotcols.append(row[1])

        journalcols = []
        for row in conn.execute("PRAGMA table_info('journaltbl')"):
            journalcols.append(row[1])

        data=[]
        for row in conn.execute("select * from journaltbl" + volstr):
            data.append(row)

        helpers.printHeader('journal table : {}'.format(len(data)))
        print tabulate.tabulate(data, headers= journalcols)

        data = []
        for row in conn.execute("select * from snapshottbl" + volstr):
            r = (row[0], (int)(row[1]/1000000) + 1, row[2])
            data.append(r)
        helpers.printHeader('snapshot table : {}'.format(len(data)))
        print tabulate.tabulate(data, headers= snapshotcols)
