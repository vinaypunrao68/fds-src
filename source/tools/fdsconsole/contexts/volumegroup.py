from svchelper import *
from svc_types.ttypes import *
from common.ttypes import *
from platformservice import *
import FdspUtils
import json
import dmtdlt
import tabulate
from thrift.transport import TTransport
from thrift.Thrift import TException


class VolumeGroupContext(Context):
    def __init__(self, *args):
        Context.__init__(self, *args)

    def getVolumes(self):
        volumes = ServiceMap.omConfig().listVolumes("")
        volumes = [v for v in volumes if not v.name.startswith('SYSTEM_')]
        return volumes

    def fetchVolumeState(self, uuid, volid):
        """
        Fetch volume state as json
        """
        svcType = self.config.getServiceApi().getServiceType(uuid)
        state=dict()
        if svcType == 'am':
            stateStr = ServiceMap.client(uuid).getStateInfo('volumegrouphandle.{}'.format(volid))
            # humanize returned json
            temp = json.loads(stateStr)
            for key in temp.keys():
                if key.isdigit():
                    dmSvcName = self.config.getServiceApi().getServiceName(long(key))
                    state[dmSvcName] = temp[key] 
                else:
                    state[key] = temp[key]
        elif svcType == 'dm':
            stateStr = ServiceMap.client(uuid).getStateInfo('volume.{}'.format(volid))
            # humanize returned json
            temp = json.loads(stateStr)
            for key in temp.keys():
                if key == "coordinator" and temp[key] != 0:
                    state[key] = self.config.getServiceApi().getServiceName(long(temp[key]))
                else:
                    state[key] = temp[key]
        else:
            raise TypeError("Invalid svctype {}".format(svcType))
        return state

    def getStateTableTemplate(self):
        return [["from", "volume", "state", "availability", "coordinator", "sequenceid"]]

    def getFillerRow(self, svc, volume, state):
        return [svc, volume, state, "N/A", "N/A", "N/A"]

    def stateJsonToTableRow(self, uuid, volume, jsonState):
        """
        Translates dm volume group json state into table row
        """
        svcType = self.config.getServiceApi().getServiceType(uuid)
        row = []
        if svcType == 'am':
            # from
            row.append(self.config.getServiceApi().getServiceName(uuid))
            # volume
            row.append(volume)
            # state
            row.append(jsonState["state"])
            # availability
            row.append("{}/{}".format(jsonState["up"],
                                      jsonState["up"] + jsonState["down"] + jsonState["syncing"]))
            # coordinator
            row.append(jsonState["coordinator"])
            # sequence id
            row.append(jsonState["sequenceid"])
        elif svcType == 'dm':
            # from
            row.append(self.config.getServiceApi().getServiceName(uuid))
            # volume
            row.append(volume)
            # state
            row.append(jsonState["state"])
            # availability
            row.append(jsonState["state"])
            # coordinator
            row.append(jsonState["coordinator"])
            # sequenceid
            row.append(jsonState["sequenceid"])
        else:
            raise TypeError("Invalid svctype {}".format(svcType))
        return row

    def getAllCoordinatorsState(self):
        """
        Queries each AM if it is hosting a coordinator
        Returns a table of coordinator state information
        """
        tbl = []
        volumes = self.getVolumes()
        for volume in volumes:
            coordinatorFound = False
            for uuid in self.config.getServiceApi().getServiceIds('am'):
                try:
                    state = self.fetchVolumeState(uuid, volume.volId)
                    if state["coordinator"] is True:
                        coordinatorFound = True
                        tbl.append(self.stateJsonToTableRow(uuid, volume.name, state))
                        break
                except ValueError:
                    pass
                except Exception as e:
                    pass
            if not coordinatorFound:
                tbl.append(self.getFillerRow("*", volume.name, "no_coordinator"))
        return tbl

    def getAllVolumesStateOnDm(self, dmsvcid):
        """
        Returns state of all volumes owned by a DM as a table
        """
        tbl = []
        for uuid in self.config.getServiceApi().getServiceIds(dmsvcid):
            volumes = self.getVolumes()

            omClient = ServiceMap.client(1028)
            msg = omClient.getDMT(0)
            dmt = dmtdlt.DMT()
            dmt.load(msg.dmt_data.dmt_data)
            
            volumes = [v for v in volumes if dmt.isVolumeOwnedByNode(v.volId, uuid)]
            for volume in volumes:
                try:
                    state = self.fetchVolumeState(uuid, volume.volId)
                    tbl.append(self.stateJsonToTableRow(uuid, volume.name, state))
                except ValueError:
                    tbl.append(self.getFillerRow(self.config.getServiceApi().getServiceName(uuid), volume.name, "no_state"))
                except Exception as e:
                    tbl.append(self.getFillerRow(self.config.getServiceApi().getServiceName(uuid), volume.name, "fetch_error"))
        return tbl

    #--------------------------------------------------------------------------------------
    @clidebugcmd
    @arg('volid', help= "volume id", type=str)
    @arg('svcid', help= "service id or name", type=str)
    def state(self, volid, svcid):
        'Gets the volume group related state from service.  At the moment am or dm are valid'
        tbl = self.getStateTableTemplate()

        if volid == "*" and svcid == "am":
            tbl = tbl + self.getAllCoordinatorsState()
            return tabulate.tabulate(tbl, headers="firstrow")
        elif volid == "*":
            tbl = tbl + self.getAllVolumesStateOnDm(svcid)
            return tabulate.tabulate(tbl, headers="firstrow")

        volid = self.config.getVolumeApi().getVolumeId(volid)
        for uuid in self.config.getServiceApi().getServiceIds(svcid):
            try:
                state = self.fetchVolumeState(uuid, volid)
                tbl.append(self.stateJsonToTableRow(uuid, volid, state))
            except ValueError:
                tbl.append(self.getFillerRow(self.config.getServiceApi().getServiceName(uuid), volid, "no_state"))
            except Exception as e:
                tbl.append(self.getFillerRow(self.config.getServiceApi().getServiceName(uuid), volid, "fetch_error"))
        return tabulate.tabulate(tbl, headers="firstrow")

    #--------------------------------------------------------------------------------------
    @clidebugcmd
    @arg('volid', help= "volume id", type=str)
    @arg('svcid', help= "service id or name", type=str)
    def statedump(self, volid, svcid):
        'Gets the volume group related state from service as json.  At the moment am or dm are valid'
        volid = self.config.getVolumeApi().getVolumeId(volid)
        for uuid in self.config.getServiceApi().getServiceIds(svcid):
            self.printServiceHeader(uuid)
            try:
                state = self.fetchVolumeState(uuid, volid)
                print (json.dumps(state, indent=2, sort_keys=True))
            except ValueError:
                print "No state available"
            except Exception, e:
                print "Failed to fetch state.  Either service is down or argument is incorrect"
        return

        
    #--------------------------------------------------------------------------------------
    @clidebugcmd
    @arg('volid', help= "volume id", type=str)
    @arg('dmuuid', help= "dm uuid", type=str)
    def forcesync(self, volid, dmuuid):
        'Forces sync on a given volume.  Volume must be offline for this to work'
        volid = self.config.getVolumeApi().getVolumeId(volid)
        svc = self.config.getPlatform();
        msg = FdspUtils.newSvcMsgByTypeId('DbgForceVolumeSyncMsg');
        msg.volId = volid
        for uuid in self.config.getServiceApi().getServiceIds(dmuuid):
            cb = WaitedCallback();
            svc.sendAsyncSvcReq(uuid, msg, cb)

            print('-->From service {}: '.format(uuid))
            if not cb.wait(30):
                print 'Failed to force sync: {}'.format(self.config.getServiceApi().getServiceName(uuid))
            elif cb.header.msg_code != 0:
                print 'Failed to force sync error: {}'.format(cb.header.msg_code)
            else:
                print "Initiated force sync"

    #--------------------------------------------------------------------------------------
    @clidebugcmd
    @arg('volid', help= "volume id", type=str)
    def offline(self, volid):
        """
        Offline volume group.  This will clear out cached coordinator id of each replica member
        of volume group.
        """
        volid = self.config.getVolumeApi().getVolumeId(volid)

        svc = self.config.getPlatform();

        omClient = ServiceMap.client(1028)
        msg = omClient.getDMT(0)
        dmt = dmtdlt.DMT()
        dmt.load(msg.dmt_data.dmt_data)

        msg = FdspUtils.newSvcMsgByTypeId('DbgOfflineVolumeGroupMsg');
        msg.volId = volid

        # first offline against AMs
        uuids = self.config.getServiceApi().getServiceIds('am')
        # then do offline against all DMs
        uuids.extend(dmt.getNodesForVolume(volid))

        for uuid in uuids:
            cb = WaitedCallback();
            svc.sendAsyncSvcReq(uuid, msg, cb)

            self.printServiceHeader(uuid)
            if not cb.wait(30):
                print 'offline timedout'
            elif cb.header.msg_code != 0:
                print 'Failed to offline volume group error: {}'.format(cb.header.msg_code)
            else:
                print "Initiated offline volume group"
