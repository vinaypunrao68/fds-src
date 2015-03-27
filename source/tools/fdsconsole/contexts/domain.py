from svchelper import *
import restendpoint

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
    @cliadmincmd
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
    @cliadmincmd
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
    @cliadmincmd
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
    @cliadmincmd
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
    @cliadmincmd
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
    @cliadmincmd
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
    @cliadmincmd
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
    @cliadmincmd
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
    @cliadmincmd
    @arg('domain_name', help="Name of the Local Domain whose Services are to be activated.")
    def activateServices(self, domain_name):
        """
        Activate the pre-defined Services on each node currently defined in the Local Domain.
        How Services are pre-defined for a given Node is currently (3/20/2015) a mystery.
        """
        try:
            return self.restApi().activateLocalDomainServices(domain_name)
        except Exception, e:
            log.exception(e)
            return 'Unable to activate Services on Local Domain: {}'.format(domain_name)

    #--------------------------------------------------------------------------------------
    @cliadmincmd
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
    @cliadmincmd
    @arg('domain_name', help="Name of the Local Domain whose Services are to be removed.")
    def removeServices(self, domain_name):
        """
        Remove the pre-defined Services on each node currently defined in the Local Domain.
        How Services are pre-defined for a given Node is currently (3/20/2015) a mystery.
        """
        try:
            return self.restApi().removeLocalDomainServices(domain_name)
        except Exception, e:
            log.exception(e)
            return 'Unable to remove Services from Local Domain: {}'.format(domain_name)
