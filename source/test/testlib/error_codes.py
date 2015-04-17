##############################################################################
# author: Philippe Ribeiro                                                   #
# email: philippe@formationds                                                #
# file: ec2.py                                                               #
##############################################################################
class Ec2FailedToInstantiate(Exception):
    
    def __init__(self, message):
        # Call the base class constructor with the parameters it needs
        super(Ec2FailedToInstantiate, self).__init__(message)

class Ec2FailedToInitialize(Exception):
    
    def __init__(self, message):
        # Call the base class constructor with the parameters it needs
        super(Ec2FailedToInitialize, self).__init__(message)