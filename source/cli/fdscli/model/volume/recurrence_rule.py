

class RecurrenceRule(object):
    '''
    Created on Apr 29, 2015
    
    @author: nate
    '''
    
    def __init__(self, frequency="DAILY", byday=None, bymonthday=None, bymonth=None, byyearday=None, byhour=None, byminute=None):
        self.frequency = frequency
        self.byday = byday
        self.byhour = byhour
        self.bymonthday = bymonthday
        self.byyearday = byyearday
        self.bymonth = bymonth
        self.byminute = byminute
        
    @property
    def frequency(self):
        return self.__frequency
    
    @frequency.setter
    def frequency(self, frequency):
        
        if frequency not in ("DAILY", "WEEKLY", "MONTHLY", "YEARLY"):
            raise TypeError()
        
        self.__frequency = frequency
        
    @property
    def byday(self):
        return self.__byday
    
    @byday.setter
    def byday(self, days):
        self.__byday = days
        
    @property
    def byhour(self):
        return self.__byhour
    
    @byhour.setter
    def byhour(self, hours):
        self.__byhour = hours
        
    @property
    def bymonthday(self):
        return self.__bymonthday
    
    @bymonthday.setter
    def bymonthday(self, monthdays):
        self.__bymonthday = monthdays
        
    @property
    def byyearday(self):
        return self.__byyearday
    
    @byyearday.setter
    def byyearday(self, yeardays):
        self.__byyearday = yeardays
        
    @property
    def bymonth(self):
        return self.__bymonth
    
    @bymonth.setter
    def bymonth(self, months):
        self.__bymonth = months
        
    @property
    def byminute(self):
        return self.__byminute
    
    @byminute.setter
    def byminute(self, byminutes):
        self.__byminute = byminutes
