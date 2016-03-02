from svchelper import *
from svc_api.ttypes import *
from common.ttypes import *
from platformservice import *
import restendpoint
import FdspUtils
import tabulate
import subprocess
import time
import humanize
import argparse
import pprint
class RootContext(Context):
    def __init__(self, *args):
        Context.__init__(self, *args)
        self.__statsApi = None
        self.metrics = {
            'ABS' : 'Ave Blob Size',
            'AOPB' : 'Ave Objects per Blob',
            'BLOBS' : 'Blobs',
            'GETS' : 'Gets',
            'LBYTES' : 'Logical Bytes',
            'LTC_SIGMA' : 'Long Term Capacity Sigma',
            'LTC_WMA' : 'Long Term Capacity WMA',
            'LTP_SIGMA' : 'Long Term Perf Sigma',
            'LTP_WMA' : 'Long Term Perf WMA',
            'MBYTES' : 'Metadata Bytes',
            'OBJECTS' : 'Objects',
            'PBYTES' : 'Physical Bytes',
            'PUTS' : 'Puts',
            'QFULL' : 'Queue Full',
            'SSD_GETS' : 'SSD Gets',
            'STC_SIGMA' : 'Short Term Capacity Sigma',
            'STC_WMA' : 'Short Term Capacity WMA',
            'STP_SIGMA' : 'Short Term Perf Sigma',
            'STP_WMA' : 'Short Term Perf WMA',
            'UBYTES' : 'Used Bytes',
        }

    def statsApi(self):
        if self.__statsApi == None:
            self.__statsApi = restendpoint.StatsEndpoint(self.config.getRestApi())
        return self.__statsApi

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
    @arg('metrics', help= "metric name", default='lbytes', nargs=argparse.REMAINDER)
    @arg('--from',dest='fromtime', help= "from time", default='1hour')
    @arg('--to', dest='totime', help= "to time", default='now', type=str)
    def stats(self, metrics, fromtime='1hour', totime='now'):
        fromtime = helpers.get_timestamp_from_human_date(fromtime, True)
        totime = helpers.get_timestamp_from_human_date(totime)
        if len(metrics) == 0:
            metrics = self.metrics.keys()
        metrics = [m.upper() for m in metrics]
        print fromtime, totime, totime-fromtime
        data = self.statsApi().getStats(series=metrics,start=fromtime, end=totime)
        #pprint.pprint(data)
        for series in data['series']:
            if 'context' not in series:
                continue
            table=[]
            prev = None
            for point in series['datapoints']:
                if prev == None:
                    prev = [int(point['x']), point['y'], 0]
                elif prev[1] != point['y']:
                    #print prev[0]
                    prev[2] = humanize.naturaldelta(int(point['x']) - prev[0])
                    prev[0] = helpers.get_date_from_timestamp(prev[0])
                    table.append(prev)
                    prev = [int(point['x']), point['y'], 0]
            if prev:
                #print prev[0]
                prev[2] = humanize.naturaldelta(int(point['x']) - prev[0])
                prev[0] = helpers.get_date_from_timestamp(prev[0])
                table.append(prev)

            print tabulate.tabulate(table, headers=['from', series['type'].lower(), 'unchanged for'])
            print
            
            
