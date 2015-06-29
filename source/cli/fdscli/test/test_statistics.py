from test.base_cli_test import BaseCliTest
import json
from utils.converters.statistics.statistics_converter import StatisticsConverter
from model.statistics.metric_query_criteria import MetricQueryCriteria
from model.volume.volume import Volume
from model.statistics.date_range import DateRange
from model.statistics.metrics import Metric
from utils.converters.statistics.metric_query_converter import MetricQueryConverter
from utils.converters.health.system_health_converter import SystemHealthConverter
from model.health.health_state import HealthState
from model.health.health_category import HealthCategory

class TestStatistics(BaseCliTest):
    '''
    Created on Jun 29, 2015
    
    @author: nate
    '''

    def test_conversion(self):
        '''
        test that we can read the JSON into our objects
        '''
        
        j_str = "{\"metadata\":[],\"series\":[{\"datapoints\":[{\"x\":0,\"y\":1},{\"x\":1,\"y\":2}],\"type\":\"GETS\"},{\"datapoints\":[{\"x\":30,\"y\":100}],\"type\":\"PUTS\"}],\"calculated\":[{\"average\":23.23},{\"total\":300}]}"
        
        j_str = json.loads(j_str)

        stats = StatisticsConverter.build_statistics_from_json(j_str)
        
        assert len(stats.series_list) == 2
        assert len(stats.series_list[0].datapoints) == 2
        assert stats.series_list[0].datapoints[1].y == 2
        assert len(stats.calculated_values) == 2
        assert stats.calculated_values[0].key == "average"
        assert stats.calculated_values[1].key == "total"
        assert stats.calculated_values[1].value == 300
        
        # go back to json?
        new_str = StatisticsConverter.to_json(stats)
        p_str = '{"series": [{"seriesType": "GETS", "datapoints": [{"y": 1, "x": 0}, {"y": 2, "x": 1}]}, {"seriesType": "PUTS", "datapoints": [{"y": 100, "x": 30}]}], "calculated": [{"average": 23.23}, {"total": 300}], "metadata": []}'
        
        assert p_str == new_str
        
    def test_query_conversion_to_josn(self):
        
        m_query = MetricQueryCriteria()
        
        volume = Volume()
        volume.name = "TestVol"
        
        m_query.contexts = [volume]
        m_query.date_range = DateRange(start=1000, end=3000)
        m_query.metrics = [Metric.GETS, Metric.PUTS, Metric.SSD_GETS]
        
        j_str = MetricQueryConverter.to_json( m_query )
        real_str = '{"contexts": [{"status": {"lastCapacityFirebreak": {"seconds": 0, "nanos": 0}, "state": "ACTIVE", "lastPerformanceFirebreak": {"seconds": 0, "nanos": 0}, "currentUsage": {"value": 0, "unit": "GB"}}, "dataProtectionPolicy": {"snapshotPolicies": [], "commitLogRetention": {"seconds": 86400, "nanos": 0}, "presetId": -1}, "uid": -1, "created": {"seconds": 0, "nanos": 0}, "settings": {"type": "OBJECT"}, "application": "None", "qosPolicy": {"priority": 7, "iopsMin": 0, "iopsMax": 0}, "mediaPolicy": "HYBRID", "name": "TestVol"}], "range": {"start": 1000, "end": 3000}, "seriesType": ["GETS", "PUTS", "SSD_GETS"]}'
        
        assert j_str == real_str
        
    def test_system_health_conversion(self):
        
        j_str = '{"overall":"ACCEPTABLE","status":[{"state":"GOOD","category":"CAPACITY","message":"No message"},{"state":"BAD","category":"SERVICES","message":"Cool message"}]}'
        
        sys_health = SystemHealthConverter.build_system_health_from_json(j_str)
        
        assert len(sys_health.health_records) == 2
        assert sys_health.overall_health == HealthState.ACCETPABLE
        assert sys_health.health_records[0].category == HealthCategory.CAPACITY
        assert sys_health.health_records[1].category == HealthCategory.SERVICES
        assert sys_health.health_records[0].state == HealthState.GOOD
        assert sys_health.health_records[1].state == HealthState.BAD
        