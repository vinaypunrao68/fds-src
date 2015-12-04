from svchelper import *
import restendpoint

class QoSPolicyContext(Context):
    def __init__(self, *args):
        Context.__init__(self, *args)
        self.__restApi = None

    def restApi(self):
        if self.__restApi == None:
            self.__restApi = restendpoint.QoSPolicyEndpoint(self.config.getRestApi())
        return self.__restApi

    #--------------------------------------------------------------------------------------
    @clicmd
    @arg('policy_name', help="Name of the new QoS Policy. Must be unique within the Global Domain.")
    @arg('iops_min', help="Minimum IOPS guarantee.")
    @arg('iops_max', help="Maximum IOPS guarantee.")
    @arg('rel_prio', help="Relative priority.")
    def create(self, policy_name, iops_min, iops_max, rel_prio):
        """
        Create a new QoS Policy.
        """
        try:
            return self.restApi().createQoSPolicy(policy_name, iops_min, iops_max, rel_prio)
        except Exception, e:
            print e
            log.exception(e)
            return 'Unable to create QoS Policy: {}'.format(policy_name)

    #--------------------------------------------------------------------------------------
    @clicmd
    def list(self):
        """
        List currently defined QoS Policies.
        """
        try:
            return self.restApi().listQoSPolicies()
        except Exception, e:
            print e
            log.exception(e)
            return 'Unable to list QoS Policies.'

    #--------------------------------------------------------------------------------------
    @clicmd
    @arg('current_policy_name', help="Current name of the QoS Policy.")
    @arg('new_policy_name', help="New name for the QoS Policy. Same as current name if name is not changing.")
    @arg('iops_min', help="New minimum IOPS guarantee.")
    @arg('iops_max', help="New maximum IOPS guarantee.")
    @arg('rel_prio', help="New relative priority.")
    def update(self, current_policy_name, new_policy_name, iops_min, iops_max, rel_prio):
        """
        Change the name of the Local Domain.
        """
        try:
            return self.restApi().updateQosPolicy(current_policy_name, new_policy_name, iops_min, iops_max, rel_prio)
        except Exception, e:
            log.exception(e)
            return 'Unable to change the QoS Policy {}'.format(current_policy_name)

    #--------------------------------------------------------------------------------------
    @clicmd
    @arg('policy_name', help="Name of the QoS Policy to be deleted.")
    def delete(self, policy_name):
        """
        Delete a QoS Policy.
        """
        try:
            return self.restApi().deleteQoSPolicy(policy_name)
        except Exception, e:
            log.exception(e)
            return 'Unable to delete QoS Policy: {}'.format(policy_name)
