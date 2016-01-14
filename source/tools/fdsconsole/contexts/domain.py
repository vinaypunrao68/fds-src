from svchelper import *
import restendpoint
import dmtdlt

# Currently (3/9/2015) does not support the full Local Domain
# interface. Just enough to match what was supported by fdscli
# is implemented.
class DomainContext(Context):
    def __init__(self, *args):
        Context.__init__(self, *args)
        self.__restApi = None

    def restApi(self):
        if self.__restApi == None:
            self.__restApi = restendpoint.DomainEndpoint(self.config.getRestApi())
        return self.__restApi

    #--------------------------------------------------------------------------------------
    @clicmd
    @arg('domain_name', help="Name of the new Local Domain. Must be unique within the Global Domain.")
    @arg('domain_site', help="Location of the new Local Domain.")
    def create(self, domain_name, domain_site):
        """
        Create a new Local Domain.
        """
        try:
            return self.restApi().createLocalDomain(domain_name, domain_site)
        except Exception, e:
            print e
            log.exception(e)
            return 'Unable to create Local Domain: {}'.format(domain_name)

    #--------------------------------------------------------------------------------------
    @clicmd
    def list(self):
        """
        List currently defined Local Domains.
        """
        try:
            return self.restApi().listLocalDomains()
        except Exception, e:
            print e
            log.exception(e)
            return 'Unable to list Local Domains.'

    #--------------------------------------------------------------------------------------
    @clicmd
    @arg('old_domain_name', help="Current name of the Local Domain.")
    @arg('new_domain_name', help="New name for the Local Domain.")
    def updateName(self, old_domain_name, new_domain_name):
        """
        Change the name of the Local Domain.
        """
        try:
            return self.restApi().updateLocalDomainName(old_domain_name, new_domain_name)
        except Exception, e:
            log.exception(e)
            return 'Unable to change the name of Local Domain {} to {}'.format(old_domain_name, new_domain_name)

    #--------------------------------------------------------------------------------------
    @clicmd
    @arg('domain_name', help="Name of the Local Domain whose site name is to be changed.")
    @arg('new_site_name', help="New name for the Local Domain's site.")
    def updateSite(self, domain_name, new_site_name):
        """
        Change the name of the Local Domain's site.
        """
        try:
            return self.restApi().updateLocalDomainSite(domain_name, new_site_name)
        except Exception, e:
            log.exception(e)
            return 'Unable to change the site name of Local Domain {} to {}'.format(domain_name, new_site_name)

    #--------------------------------------------------------------------------------------
    @clicmd
    @arg('domain_name', help="Name of the Local Domain whose Throttle is to be set.")
    @arg('throttle_level', help="The new throttle level to which to set the Local Domain.")
    def setThrottle(self, domain_name, throttle_level):
        """
        Set the throttle level in the Local Domain.
        """
        try:
            return self.restApi().setThrottle(domain_name, throttle_level)
        except Exception, e:
            log.exception(e)
            return 'Unable to set the throttle level in Local Domain {} to {}'.format(domain_name, throttle_level)

    #--------------------------------------------------------------------------------------
    @clicmd
    @arg('domain_name', help="Name of the Local Domain whose Scavenger is to be set.")
    @arg('scavenger_action', help="The Scavenger action to set: enable, disable, start, stop.")
    def setScavenger(self, domain_name, scavenger_action):
        """
        Set the scavenger action in the Local Domain.
        """
        try:
            return self.restApi().setScavenger(domain_name, scavenger_action)
        except Exception, e:
            log.exception(e)
            return 'Unable to set the scavenger action in Local Domain {} to {}'.format(domain_name, scavenger_action)

    #--------------------------------------------------------------------------------------
    @clicmd
    @arg('domain_name', help="Name of the Local Domain to be started.")
    def startup(self, domain_name):
        """
        Startup the Local Domain.
        """
        try:
            return self.restApi().startupLocalDomain(domain_name)
        except Exception, e:
            log.exception(e)
            return 'Unable to start Local Domain: {}'.format(domain_name)

    #--------------------------------------------------------------------------------------
    @clicmd
    @arg('domain_name', help="Name of the Local Domain to be shutdown.")
    def shutdown(self, domain_name):
        """
        Shutdown the Local Domain.
        """
        try:
            return self.restApi().shutdownLocalDomain(domain_name)
        except Exception, e:
            log.exception(e)
            return 'Unable to shutdown Local Domain: {}'.format(domain_name)

    #--------------------------------------------------------------------------------------
    @clicmd
    @arg('domain_name', help="Name of the Local Domain to be deleted.")
    def delete(self, domain_name):
        """
        Delete a Local Domain.
        """
        try:
            return self.restApi().deleteLocalDomain(domain_name)
        except Exception, e:
            log.exception(e)
            return 'Unable to delete Local Domain: {}'.format(domain_name)

    #--------------------------------------------------------------------------------------
    @clicmd
    @arg('domain_name', help="Name of the Local Domain whose Services are to be activated.")
    @arg('services', help="Optional comma-separated names of the Services to be activated. SM, DM, and/or AM. "
                          "If not provided, all Services defined for any given Node in the Local Domain are "
                          "activated. And in this case, if a Node has no defined Services, then all three "
                          "will be activated.", default="", nargs="?")
    def activateServices(self, domain_name, services):
        """
        Activate the given or pre-defined Services on each node currently defined in the Local Domain.
        How Services are pre-defined for a given Node is currently (3/20/2015) a mystery although one way to
        do it might be to use the single-Node Service activation endpoint specifying those Services you wish
        to be defined for the Node.
        """
        sm = False
        dm = False
        am = False

        # Which Services, if any, are specified?
        num_services = 0
        if services != "":
            service_list = services.split(",")
            for service in service_list:
                if service.lower() == "sm":
                    sm = True
                    num_services += 1
                elif service.lower() == "dm":
                    dm = True
                    num_services += 1
                elif service.lower() == "am":
                    am = True
                    num_services += 1
                else:
                    return "Services should appear as a comma-separated list of some combination of " \
                           "SM, DM, and AM. {} is incorrect".format(services)
        numpms = len(self.config.getServiceApi().getServiceIds("pm"))
        total_expected_services = 1 + numpms + numpms * num_services
        print 'num nodes: {} - total services:{}'.format(numpms, total_expected_services)
        try:
            condition = True
            count = 0
            while condition:
                retVal = self.restApi().activateLocalDomainServices(domain_name, sm, dm, am)
                num_services = len(self.config.getServiceApi().getServiceList(true))
                count += 1                
                print '{} : expected services:{} and {} showed up'.format(count, total_expected_services, num_services)
                condition = (count < 40) and (num_services < total_expected_services)
                if condition:
                    time.sleep(1)

            if num_services < total_expected_services:
                print 'please check .. expected services:{} but only {} showed up'.format(total_expected_services, num_services)
        except Exception, e:
            log.exception(e)
            return 'Unable to activate Services on Local Domain: {}'.format(domain_name)

    #--------------------------------------------------------------------------------------
    @clicmd
    @arg('domain_name', help="Name of the Local Domain whose Services are to be listed.")
    def listServices(self, domain_name):
        """
        List currently defined Services for the named Local Domain.
        """
        try:
            return self.restApi().listLocalDomainServices(domain_name)
        except Exception, e:
            print e
            log.exception(e)
            return 'Unable to list Services for Local Domain {}.'.format(domain_name)

    #--------------------------------------------------------------------------------------
    @clicmd
    @arg('domain_name', help="Name of the Local Domain whose Services are to be removed.")
    @arg('services', help="Optional comma-separated names of the Services to be removed. SM, DM, and/or AM. "
                          "If not provided, all Services defined for any given Node in the Local Domain are "
                          "removed.", default="", nargs="?")
    def removeServices(self, domain_name, services):
        """
        Remove the specified or pre-defined Services on each node currently defined in the Local Domain.
        How Services are pre-defined for a given Node is currently (3/20/2015) a mystery although one way to
        do it might be to use the single-Node Service activation endpoint specifying those Services you wish
        to be defined for the Node.
        """
        sm = False
        dm = False
        am = False

        # Which Services, if any, are specified?
        if services != "":
            service_list = services.split(",")
            for service in service_list:
                if service.lower() == "sm":
                    sm = True
                elif service.lower() == "dm":
                    dm = True
                elif service.lower() == "am":
                    am = True
                else:
                    return "Services should appear as a comma-separated list of some combination of " \
                           "SM, DM, and AM. {} is incorrect".format(services)

        try:
            return self.restApi().removeLocalDomainServices(domain_name, sm, dm, am)
        except Exception, e:
            log.exception(e)
            return 'Unable to remove Services from Local Domain: {}'.format(domain_name)

    #--------------------------------------------------------------------------------------
    @clidebugcmd
    def showdmt(self):
        'display dmt info'
        omClient = ServiceMap.client(1028)
        msg = omClient.getDMT(0)
        #print msg
        dmt = dmtdlt.DMT()
        dmt.load(msg.dmt_data.dmt_data)
        dmt.dump()

    #--------------------------------------------------------------------------------------
    @clidebugcmd
    def showdlt(self):
        'display dlt info'
        omClient = ServiceMap.client(1028)
        msg = omClient.getDLT(0)
        #print msg
        dlt = dmtdlt.DLT()
        dlt.load(msg.dlt_data.dlt_data)
        dlt.dump()

    #--------------------------------------------------------------------------------------
    @clidebugcmd
    @arg('volume', help='-volume name/id')
    def whereisvolume(self, volume):
        'display where the volume meta is located'
        omClient = ServiceMap.client(1028)
        msg = omClient.getDMT(0)
        dmt = dmtdlt.DMT()
        dmt.load(msg.dmt_data.dmt_data)
        volId = self.config.getVolumeApi().getVolumeId(volume)
        count = 1
        for uuid in dmt.getNodesForVolume(volId) :
            print '{} : {}' .format(count, self.config.getServiceApi().getServiceName(uuid))
            count += 1

    #--------------------------------------------------------------------------------------
    @clidebugcmd
    @arg('objid', help='-object id')
    def whereisobject(self, objid):
        'display where the volume meta is located'

        if len(objid) != 40:
            print '{} does not seem to be a valid object id'.format(objid)
            return
        omClient = ServiceMap.client(1028)
        msg = omClient.getDLT(0)
        dlt = dmtdlt.DLT()
        dlt.load(msg.dlt_data.dlt_data)
        count = 1
        for uuid in dlt.getNodesForObject(objid) :
            print '{} : {}' .format(count, self.config.getServiceApi().getServiceName(uuid))
            count += 1
