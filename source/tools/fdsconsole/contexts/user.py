from  svchelper import *
import restendpoint

class UserContext(Context):
    def __init__(self, *args):
        Context.__init__(self, *args)
        self.__restApi = None

    def restApi(self):
        if self.__restApi == None:
            self.__restApi = restendpoint.UserEndpoint(self.config.getRestApi())
        return self.__restApi

    #--------------------------------------------------------------------------------------
    @clicmd
    def list(self):
        'show the list of users in the system'
        try:
            users = self.restApi().listUsers()
            return tabulate([(item['id'], item['identifier'], 'YES' if item['isFdsAdmin'] else 'NO')
                              for item in sorted(users, key=itemgetter('identifier'))  ],
                            headers=['id', 'identifier', 'admin'], tablefmt=self.config.getTableFormat())
        except Exception, e:
            log.exception(e)
            return 'unable to get user list'

    #--------------------------------------------------------------------------------------
    @clicmd
    @arg('name', help= "user name")
    @arg('password', help= "password for this user")
    def create(self, name, password):
        'create a new user'
        try:
            return self.restApi().createUser(name, password)
        except Exception, e:
            log.exception(e)
            return 'unable to create user: {}'.format(name)
    
    #--------------------------------------------------------------------------------------
    @clicmd
    @arg('name', help= "name of the user")
    @arg('password', help= "password")
    def update_password(self, name, password):
        'set the password for a user'
        try:
            return self.restApi().updatePassword(name, password)
        except Exception, e:
            log.exception(e)
            return 'unbale to update password for : {}'.format(name)
