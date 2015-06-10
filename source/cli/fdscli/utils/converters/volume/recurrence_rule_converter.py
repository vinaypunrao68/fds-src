import json
from model.volume.recurrence_rule import RecurrenceRule

class RecurrenceRuleConverter(object):
    '''
    Created on Apr 29, 2015
    
    @author: nate
    '''
    
    @staticmethod
    def build_rule_from_json( jString ):
        
        rule = RecurrenceRule()
        
        if not isinstance( jString, dict ):
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
        
        if ( rule.byday is not None ):
            d["BYDAY"] = rule.byday
            
        if ( rule.bymonth is not None ):
            d["BYMONTH"] = rule.bymonth
            
        if ( rule.byhour is not None ):
            d["BYHOUR"] = rule.byhour
            
        if ( rule.bymonthday is not None ):
            d["BYMONTHDAY"] = rule.bymonthday
        
        if ( rule.byyearday is not None ):
            d["BYYEARDAY"] = rule.byyearday
            
        if ( rule.byminute is not None ):
            d["BYMINUTE"] = rule.byminute 
            
        result = json.dumps( d )
        
        return result
