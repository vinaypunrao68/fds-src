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
            return self.restApi().createDomain(domain_name, domain_site)
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
            return self.restApi().listDomains()
        except Exception, e:
            print e
            log.exception(e)
            return 'Unable to list Local Domains.'

    #--------------------------------------------------------------------------------------
    @cliadmincmd
    @arg('domain_name', help="Name of the Local Domain whose Services are to be listed.")
    def listServices(self, domain_name):
        """
        List currently defined Services for the named Local Domain.
        """
        try:
            return self.restApi().listServices(domain_name)
        except Exception, e:
            print e
            log.exception(e)
            return 'Unable to list Services for Local Domain {}.'.format(domain_name)

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
            return 'Unable to set the throttle level in Local Domain: {}'.format(domain_name)

    #--------------------------------------------------------------------------------------
    @cliadmincmd
    @arg('domain_name', help="Name of the Local Domain whose Services are to be activated.")
    @arg('sm', help="True if Storage Manager services are to be activated on each node in the Local Domain.")
    @arg('dm', help="True if Data Manager services are to be activated on each node in the Local Domain.")
    @arg('am', help="True if Access Manager services are to be activated on each node in the Local Domain.")
    def activate(self, domain_name, sm, dm, am):
        """
        Activate the indicated Services on each node currently defined in the Local Domain.
        """
        try:
            return self.restApi().activateDomain(domain_name, sm, dm, am)
        except Exception, e:
            log.exception(e)
            return 'Unable to activate Local Domain: {}'.format(domain_name)

    #--------------------------------------------------------------------------------------
    @cliadmincmd
    @arg('domain_name', help="Name of the Local Domain to be shut down.")
    def shutdown(self, domain_name):
        """
        Shutdown a Local Domain.
        """
        try:
            return self.restApi().shutdownDomain(domain_name)
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
            return self.restApi().deleteDomain(domain_name)
        except Exception, e:
            log.exception(e)
            return 'Unable to delete Local Domain: {}'.format(domain_name)
