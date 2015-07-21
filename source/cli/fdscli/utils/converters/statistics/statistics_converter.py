from model.statistics.statistics import Statistics
from model.statistics.series import Series
from model.statistics.datapoint import Datapoint
import json
from model.statistics.calculated import Calculated

class StatisticsConverter(object):
    '''
    Created on Jun 29, 2015
    
    @author: nate
    '''

    @staticmethod
    def build_statistics_from_json( j_str ):
        
        stats = Statistics()
        
        j_series = j_str.pop("series")
        
        for j_seri in j_series:
            #if you don't reset this, it picks up the last values!!
            series = Series(a_type=None)
            series.type = j_seri.pop("type", series.type)
            
            j_dps = j_seri.pop("datapoints", [])
            
            for j_dp in j_dps:
                datapoint = Datapoint()
                datapoint.x = j_dp.pop("x", datapoint.x)
                datapoint.y = j_dp.pop("y", datapoint.y)
                series.datapoints.append( datapoint )
            # end of for
                
            stats.series_list.append( series )
            
        # end of for
        
        j_calcs = j_str.pop("calculated")
        
        for j_calc in j_calcs:
            calc_pair = Calculated()
            calc_pair.key = j_calc.keys()[0]
            calc_pair.value = j_calc[calc_pair.key]
            
            stats.calculated_values.append( calc_pair )
        
        return stats
    
    @staticmethod
    def to_json( stats ):
        
#         if not isinstance(stats, Statistics):
#             raise TypeError()
        
        j_str = dict()
        
        j_series = []
        
        for seri in stats.series_list:
            j_seri = json.loads( StatisticsConverter.series_to_json(seri) )
            j_series.append( j_seri )
            
        j_str["series"] = j_series
        j_str["metadata"] = []
    
        j_calcs = []
    
        for calc in stats.calculated_values:
            d = dict()
            d[calc.key] = calc.value
            j_calcs.append( d )
            
        j_str["calculated"] = j_calcs
        
        j_str = json.dumps(j_str)
        
        return j_str
        
    @staticmethod 
    def series_to_json(series):
        
#         if not isinstance(series, Series):
#             raise TypeError()
        
        j_series = dict()
        
        j_series["seriesType"] = series.type
        
        j_dps = []
        
        for dp in series.datapoints:
            j_dp = json.loads(StatisticsConverter.datapoint_to_json(dp))
            j_dps.append( j_dp )
        
        j_series["datapoints"] = j_dps
        
        j_series = json.dumps(j_series)
        
        return j_series
        
    @staticmethod
    def datapoint_to_json(dp):
        
#         if not isinstance(dp, Datapoint):
#             raise TypeError()
        
        j_dp = dict()
        
        j_dp["x"] = dp.x
        j_dp["y"] = dp.y
        
        j_dp = json.dumps( j_dp )
        
        return j_dp
        