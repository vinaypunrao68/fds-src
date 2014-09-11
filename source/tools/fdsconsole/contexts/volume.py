from  svchelper import *

from fdsconsole.context import Context
from fdsconsole.decorators import *
from argh import *
from FDS_ProtocolInterface.ttypes import *
from snapshot.ttypes import *


class VolumeContext(Context):
    def __init__(self, *args):
        Context.__init__(self, *args)
        ServiceMap.init(self.config.getSystem('host'), self.config.getSystem('port'))

    def get_context_name(self):
        return "volume"

    #--------------------------------------------------------------------------------------
    @clicmd
    def list(self):
        try:
            volumes = ServiceMap.omConfig().listVolumes("")
            return volumes
        except Exception, e:
            log.exception(e)
            return 'unable to get volume list'

    #--------------------------------------------------------------------------------------
    @cliadmincmd
    @arg('vol-name', help= "-Volume name  of the clone")
    @arg('clone-name', help= "-name of  the  volume clone")
    @arg('policy-name', help= "-clone policy name")
    def clone(self, vol_name, clone_name, policy_name):
        try:
            volume_id  = ServiceMap.omConfig().getVolumeId(vol_name);
            ServiceMap.omConfig().cloneVolume(volume_id, policy_name, clone_name )
            return 'Success'
        except Exception, e:
            log.exception(e)
            return 'create clone failed: {}'.format(vol_name)
    
    #--------------------------------------------------------------------------------------
    @cliadmincmd
    @arg('vol-name', help= "-volume name  of the clone")
    @arg('clone-name', help= "-name of  the  volume clone for restore")
    def restore(self, vol_name, clone_name):
        try:
            volume_id  = ServiceMap.omConfig().getVolumeId(vol_name);
            ServiceMap.omConfig().restoreClone(volume_id, snapshotName)
            return 'Success'
        except Exception, e:
            log.exception(e)
            return 'restore clone failed: {}'.format(vol_name)

#======================================================================================

class SnapshotContext(Context):
    def __init__(self, *args):
        Context.__init__(self, *args)
        ServiceMap.init(self.config.getSystem('host'), self.config.getSystem('port'))

    def get_context_name(self):
        return "snapshot"

    #--------------------------------------------------------------------------------------
    @clicmd    
    @arg('vol-name', help= "-list snapshot for Volume name")
    def list(vol_name):
        try:
            volume_id  = ServiceMap.omConfig().getVolumeId(vol_name);
            snapshot = ServiceMap.omConfig().listSnapshots(volume_id)
            return tabulate([(item.snapshotName, item.volumeId, item.snapshotId, item.snapshotPolicyId, time.ctime((item.creationTimestamp)/1000)) for item in snapshot],
                            headers=['Snapshot-name', 'volume-Id', 'Snapshot-Id', 'policy-Id', 'Creation-Time'])
        except Exception, e:
            log.exception(e)
            return ' list snapshot polcies  snapshot polices for volume failed: {}'.format(vol_name)


#======================================================================================

class SnapshotPolicyContext(Context):
    def __init__(self, *args):
        Context.__init__(self, *args)
        ServiceMap.init(self.config.getSystem('host'), self.config.getSystem('port'))

    def get_context_name(self):
        return "policy"
    
    #--------------------------------------------------------------------------------------
    @cliadmincmd
    @arg('policy-name', help= "-snapshot policy name ")
    @arg('recurrence-rule', help= "- ical Rule format: FREQ=MONTHLY;BYDAY=FR;BYHOUR=23;BYMINUTE=30 ")
    @arg('retention-time', type=long, help= "-retension time for the snapshot")
    def create(self, policy_name, recurrence_rule, retention_time):
        try:
            snap_policy = SnapshotPolicy(
                id                    =   0,
                recurrenceRule        =   recurrence_rule,
                policyName            =   policy_name,
                retentionTimeSeconds  =   retention_time
            )

            policy_id = ServiceMap.omConfig().createSnapshotPolicy(snap_policy)
            return  ' Successfully created  snapshot policy: {}'.format(policy_id)
        except Exception, e:
            print e
            log.exception(e)
            return 'creating snapshot policy failed: {}'.format(policy_name)

    #--------------------------------------------------------------------------------------
    @cliadmincmd
    @arg('policy-id', type=long, help= "-snapshot policy Identifier")
    def delete(self, policy_id):
        try:
            ServiceMap.omConfig().deleteSnapshotPolicy(policy_id)
            return 'Success'
        except Exception, e:
            log.exception(e)
            return ' delete  snapshot policy failed: {}'.format(policy_id)

    #--------------------------------------------------------------------------------------
    @clicmd
    def list(self, volumeName=None):
        try:
            if volumeName == None:
                policy_list = ServiceMap.omConfig().listSnapshotPolicies(0)
            else:
                volume_id  = ServiceMap.omConfig().getVolumeId(volumeName);
                policy_list = ServiceMap.omConfig().listSnapshotPoliciesForVolume(volume_id)
            return tabulate([(item.policyName, item.recurrenceRule, item.id,  item.retentionTimeSeconds) for item in policy_list],
                            headers=['Policy-Name', 'Policy-Rule', 'Policy-Id', 'Retension-Time'])
        except Exception, e:
            print e
            log.exception(e)
            return ' list  snapshot policy failed: {}'.format(volumeName)

    #--------------------------------------------------------------------------------------
    @clicmd
    @arg('vol-name', help= "-Volume name for attaching snap policy")
    @arg('policy-id', type=long,  help= "-snap shot policy id")
    def attach(self, vol_name, policy_id):
        try:
            volume_id  = ServiceMap.omConfig().getVolumeId(vol_name);
            ServiceMap.omConfig().attachSnapshotPolicy(volume_id, policy_id)
            return 'Success'
        except Exception, e:
            log.exception(e)
            return 'attach snap policy  failed: {}'.format(vol_name)
    
    #--------------------------------------------------------------------------------------
    @clicmd
    @arg('vol-name', help= "-Volume name for detaching snap policy")
    @arg('policy-id', type=long,  help= "-snap shot policy name")
    def detach(self, vol_name, policy_id):
        try:
            volume_id  = ServiceMap.omConfig().getVolumeId(vol_name);
            ServiceMap.omConfig().detachSnapshotPolicy(volume_id, policy_id)
            return 'Success'
        except Exception, e:
            log.exception(e)
            return 'detach snap policy failed: {}'.format(vol_name)

    #--------------------------------------------------------------------------------------
    @clicmd
    @arg('policy-id', type=long, help= "-snap shot policy id")
    def list_volumes(policy_id):
        try:
            # get the  OM client  handler
            client = svc_map.omConfig()
            # invoke the thrift  interface call
            volume_list =  client.listVolumesForSnapshotPolicy(policy_id)
            return tabulate([(item) for item in volume_list],
                            headers=['Volume-Id'])
        except Exception, e:
            log.exception(e)
            return ' list Volumes for snapshot policy failed: {}'.format(policy_id)


#======================================================================================
