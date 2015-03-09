import hashlib

class User(object):
    '''
    Create a User (could be a mock user). This user contains an unique
    identifier (created with a MD5 hash, based on his id)
    
    Attributes:
    -----------
    id : int
        the user id.
    name : str
        the user name. Could be empty if mock user.
    hash : md5
        a md5 hash, which is shared by all members of the class.
    '''
    hash = hashlib.md5()
    
    def __init__(self, id, name=""):
        self.id = id
        self.name = name
        self.hash.update(str(self.id))
        self.identifier = self.hash.digest()
            
            