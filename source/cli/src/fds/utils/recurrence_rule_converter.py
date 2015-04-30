import json
from fds.model.recurrence_rule import RecurrenceRule

class RecurrenceRuleConverter(object):
    '''
    Created on Apr 29, 2015
    
    @author: nate
    '''
    
    @staticmethod
    def build_rule_from_json( jString ):
        
        rule = RecurrenceRule()
        
        if isinstance( jString, dict ) == False:
            jString = json.loads( jString )
        
        rule.frequency = jString.pop( "FREQ", rule.frequency )
        rule.byday = jString.pop( "BYDAY", None )
        rule.byhour = jString.pop( "BYHOUR", None )
        rule.bymonth = jString.pop( "BYMONTH", None )
        rule.byyearday = jString.pop( "BYYEARDAY", None )
        rule.bymonthday = jString.pop( "BYMONTHDAY", None )
        rule.byminute = jString.pop( "BYMINUTE", None )
        
        return rule
    
    @staticmethod
    def to_json( rule ):
        
        d = dict()
        
        d["FREQ"] = rule.frequency
        
        if ( rule.byday != None ):
            d["BYDAY"] = rule.byday
            
        if ( rule.bymonth != None ):
            d["BYMONTH"] = rule.bymonth
            
        if ( rule.byhour != None ):
            d["BYHOUR"] = rule.byhour
            
        if ( rule.bymonthday != None ):
            d["BYMONTHDAY"] = rule.bymonthday
        
        if ( rule.byyearday != None ):
            d["BYYEARDAY"] = rule.byyearday
            
        if ( rule.byminute != None ):
            d["BYMINUTE"] = rule.byminute 
            
        result = json.dumps( d )
        
        return result