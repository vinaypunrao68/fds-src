##############################################################################
# author: Philippe Ribeiro                                                   #
# email: philippe@formationds                                                #
# file: provision.py                                                         #
##############################################################################

class Provision(object):
    '''
    Specify which type of provision will the user be instantiating in order to
    create their FDS nodes.
    Uses EC2 by default. Other provisions will be supported later.
    '''
    instance_types = ("ec2")
    
    def __init__(self, type="ec2"):
        assert type in instance_type
        self.type = type