from argparse import ArgumentTypeError

class SnapshotPolicyValidator(object):
    '''
    Created on Apr 29, 2015
    
    @author: nate
    '''

    @staticmethod
    def day_of_month_values( value ):
        
        value = int(value)
        
        if value < 1 or value > 31:
            raise ArgumentTypeError( "The value for day must be between 1 and 31.")
            
        return value
    
    @staticmethod
    def day_of_year_values( value ):
        
        value = int(value)
        
        if value < 1 or value > 365:
            raise ArgumentTypeError( "The value for day must be between 1 and 365.")
            
        return value    
    
    @staticmethod
    def week_value( value ):
        
        value = int(value)
        
        if value < 1 or value > 52 :
                raise ArgumentTypeError( "The value for day must be between 1 and 52.")
            
        return value      
    
    @staticmethod
    def month_value( value ):
        
        value = int(value)
        
        if value < 1 or value > 12 :
                raise ArgumentTypeError( "The value for month must be between 1 and 12.")
            
        return value        
    
    @staticmethod
    def hour_value( value ):
        
        value = int(value)
        
        if value < 0 or value > 23 :
                raise ArgumentTypeError( "The value for hour must be between 0 and 23.")
            
        return value    
    
    @staticmethod
    def minute_value( value ):
        
        value = int(value)
        
        if value < 0 or value > 59 :
                raise ArgumentTypeError( "The value for minute must be between 0 and 59.")
            
        return value 
