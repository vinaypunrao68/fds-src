from  svchelper import *
from fdslib.pyfdsp.apis import ttypes
import restendpoint

class TenantContext(Context):
    def __init__(self, *args):
        Context.__init__(self, *args)
        self.restApi = restendpoint.TenantEndpoint(self.config.rest) 

    #--------------------------------------------------------------------------------------
    @clicmd
    def list(self):
        'show the list of users in the system'
        try:
            tenants = self.restApi.listTenants()
            return tabulate([(item['id'], item['name'])
                             for item in sorted(tenants, key=itemgetter('name'))  ],
                            headers=['id', 'name'], tablefmt=self.config.getTableFormat())
        except Exception, e:
            log.exception(e)
            return '[ERROR] : unable to get tenant list'

    #--------------------------------------------------------------------------------------
    @cliadmincmd
    @arg('name', help= "tenant name")
    def create(self, name, password):
        'create a new tenant'
        try:
            return self.restApi.createTenant(name)
        except Exception, e:
            log.exception(e)
            return '[ERROR] : unable to create tenant: {}'.format(name)
    
    #--------------------------------------------------------------------------------------
    @cliadmincmd
    @arg('user', help= "name of the user")
    @arg('tenant', help= "password")
    def assign_user(self, user, tenant):
        'assign the user to a tenant'
        try:
            return self.restApi.assignUserToTenant(user, tenant)
        except Exception, e:
            log.exception(e)
            return 'unbale to assign user : {} to tenant : {}'.format(user, tenant)
