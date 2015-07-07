from model.statistics.date_range import DateRange
import json

class DateRangeConverter(object):
    '''
    Created on Jun 29, 2015
    
    @author: nate
    '''

    @staticmethod
    def to_json(date_range):
        
#         if not isinstance(date_range,DateRange):
#             raise TypeError()
#         
        dr_json = dict()
        
        dr_json["start"] = date_range.start
        dr_json["end"] = date_range.end
        
        dr_json = json.dumps(dr_json)
        
        return dr_json
    
    @staticmethod
    def build_date_range_from_json(j_str):
        
        date_range = DateRange()
        
        date_range.start = j_str.pop("start", date_range.start)
        date_range.end = j_str.pop("end", date_range.end)
        
        return date_range