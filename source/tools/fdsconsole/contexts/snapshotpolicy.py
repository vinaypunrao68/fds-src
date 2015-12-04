from svchelper import *
import pyfdsp.config_types
import pyfdsp.config_types.ttypes

class SnapshotPolicyContext(Context):
    def __init__(self, *args):
        Context.__init__(self, *args)
    
    #--------------------------------------------------------------------------------------
    @clicmd
    @arg('policy-name', help= "-snapshot policy name ")
    @arg('recurrence-rule', help= "- ical Rule format: FREQ=MONTHLY;BYDAY=FR;BYHOUR=23;BYMINUTE=30 ")
    @arg('retention-time', type=long, help= "-retension time for the snapshot")
    def create(self, policy_name, recurrence_rule, retention_time):
        'create a new snaphot policy'
        try:
            snap_policy = pyfdsp.config_types.ttypes.SnapshotPolicy(
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
    @clicmd
    @arg('policy-id', type=long, help= "-snapshot policy Identifier")
    def delete(self, policy_id):
        'delete a snapshot policy'
        try:
            ServiceMap.omConfig().deleteSnapshotPolicy(policy_id)
            return 'Success'
        except Exception, e:
            log.exception(e)
            return ' delete  snapshot policy failed: {}'.format(policy_id)

    #--------------------------------------------------------------------------------------
    @clicmd
    @arg('vol-name', help= "volume name", nargs="?")
    def list(self, vol_name):
        'list all snapshot policies for a volume'
        try:
            if vol_name == None:
                policy_list = ServiceMap.omConfig().listSnapshotPolicies(0)
            else:
                volume_id  = ServiceMap.omConfig().getVolumeId(vol_name);
                policy_list = ServiceMap.omConfig().listSnapshotPoliciesForVolume(volume_id)
            return tabulate([(item.policyName, item.recurrenceRule, item.id,  item.retentionTimeSeconds) for item in policy_list],
                            headers=['Policy-Name', 'Policy-Rule', 'Policy-Id', 'Retension-Time'], tablefmt=self.config.getTableFormat())
        except Exception, e:
            print e
            log.exception(e)
            return ' list  snapshot policy failed: {}'.format(vol_name)

    #--------------------------------------------------------------------------------------
    @clicmd
    @arg('vol-name', help= "-Volume name for attaching snap policy")
    @arg('policy-id', type=long,  help= "-snap shot policy id")
    def attach(self, vol_name, policy_id):
        'attach a snapshot policy to a volume'
        try:
            volume_id  = ServiceMap.omConfig().getVolumeId(vol_name);
            ServiceMap.omConfig().attachSnapshotPolicy(volume_id, policy_id)
            return 'Success'
        except Exception, e:
            log.exception(e)
            return 'attach snap policy  failed: {}'.format(vol_name)
    
    #--------------------------------------------------------------------------------------
    @clidebugcmd
    @arg('vol-name', help= "-Volume name for detaching snap policy")
    @arg('policy-id', type=long,  help= "-snap shot policy name")
    def detach(self, vol_name, policy_id):
        'detach a snapshot policy from a volume'
        try:
            volume_id  = ServiceMap.omConfig().getVolumeId(vol_name);
            ServiceMap.omConfig().detachSnapshotPolicy(volume_id, policy_id)
            return 'Success'
        except Exception, e:
            log.exception(e)
            return 'detach snap policy failed: {}'.format(vol_name)

    #--------------------------------------------------------------------------------------
    @clidebugcmd
    @arg('policy-id', type=long, help= "-snap shot policy id")
    def volumes(self, policy_id):
        'show volumes with a specific snapshot policy'
        try:
            volume_list =  ServiceMap.omConfig().listVolumesForSnapshotPolicy(policy_id)
            return tabulate([(policy_id, item) for item in volume_list],
                            headers=['Policy-Id','Volume-Id'], tablefmt=self.config.getTableFormat())
        except Exception, e:
            log.exception(e)
            return ' list Volumes for snapshot policy failed: {}'.format(policy_id)


#======================================================================================
