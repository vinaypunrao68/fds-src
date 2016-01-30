from svchelper import *
from svc_api.ttypes import *
from common.ttypes import *
from platformservice import *
import FdspUtils
import tabulate

class TimelineContext(Context):

    def __init(self, *args):
        Context.__init__(self, *args)
    '''
    This is the first context ..
    Currently it is a dummy.. Later all shared functionality will be here.
    '''
    def get_context_name(self):
        return "timeline"

    @clidebugcmd
    @arg('volume', help= "volume id/name", type=str, default=None, nargs='?')
    def dump(self, volume):
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

    @clidebugcmd
    def ping(self):
        're ping timeline queues in om'
        for uuid in self.config.getServiceApi().getServiceIds("om"):
            msg = FdspUtils.newSvcMsgByTypeId(FDSPMsgTypeId.GenericCommandMsgTypeId)
            msg.command="timeline.queue.ping";
            cb = WaitedCallback()
            print 'sending timeline.queue.ping to {}'.format(self.config.getServiceApi().getServiceName(uuid))
            self.config.getPlatform().sendAsyncSvcReq(uuid, msg, cb)
