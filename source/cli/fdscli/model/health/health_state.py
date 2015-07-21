
class HealthState(object):
    '''
    Created on Jun 29, 2015
    
    @author: nate
    '''

    GOOD = "GOOD"
    BAD = "BAD"
    ACCETPABLE = "ACCEPTABLE"
    EXCELLENT = "Excellent"
    MARGINAL = "Marignal"
    LIMITED = "Limited"
    OKAY = "OKAY"
    
    MESSAGES = dict()    
    MESSAGES["l_services_good"] = "All expected services are running properly"
    MESSAGES["l_services_not_good"] = "Some services are not running as expected"
    MESSAGES["l_services_bad"] = "Necessary services are not running and require attention"
    MESSAGES["l_capacity_good"] = "System capacity has adequate free space"
    MESSAGES["l_capacity_not_good_threshold"] = "System capacity is above 80%"
    MESSAGES["l_capacity_not_good_rate"] = "System capacity is predicated to run out in less than 30 days"
    MESSAGES["l_capacity_bad_threshold"] = "System capacity is above 90%"
    MESSAGES["l_capacity_bad_rate"] = "System capacity is predicted to run out in less than 1 week"
    MESSAGES["l_firebreak_good"] = "No firebreak events in the last 24 hours"
    MESSAGES["l_firebreak_not_good"] = "Some volumes have experienced a firebreak in the last 24 hours"
    MESSAGES["l_firebreak_bad"] = "Most volumes have experience a firebreak in the last 24 hours"