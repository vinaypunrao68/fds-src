from base_cli_test import BaseCliTest
from model.volume import Volume
from model.node import Node
from model.service import Service
from model.snapshot import Snapshot
from model.snapshot_policy import SnapshotPolicy
from model.domain import Domain

import time
from services.response_writer import ResponseWriter
from model.recurrence_rule import RecurrenceRule

class TestResponseWriterPrep(BaseCliTest):
    '''
    Created on May 1, 2015
    
    @author: nate
    '''

    def test_prep_volume(self):
        '''
        Test that the volume table preparation is done properly
        '''
        
        volumes = []
        volume = Volume()
        volume.id = 400
        volume.name = "TestVol"
        volume.continuous_protection = 100000
        volume.iops_guarantee = 0
        volume.iops_limit = 100
        volume.priority = 9
        volume.current_size = 30
        volume.current_units = "GB"
        volume.media_policy = "SSD_ONLY"
        volume.last_capacity_firebreak = 0
        volume.last_performance_firebreak = 0
        volume.state = "ACTIVE"
        volume.tenant_id = 12
        volumes.append( volume )
        
        table = ResponseWriter.prep_volume_for_table(self.auth, volumes)
        
        row = table[0]
        
        assert row["ID"] == volume.id
        assert row["Name"] == volume.name
        assert row["State"] == volume.state
        assert row["Media Policy"] == volume.media_policy
        assert row["IOPs Guarantee"] == "None"
        assert row["IOPs Limit"] == volume.iops_limit
        assert row["Tenant ID"] == volume.tenant_id
        assert row["Last Firebreak"] == "Never"
        assert row["Last Firebreak Type"] == ""
        assert row["Usage"] == "30 GB"
        
    def test_prep_nodes(self):
        '''
        Test that the node preparation is done properly
        '''
        node = Node()
        node.name = "FakeNode"
        node.ip_v4_address = "10.12.14.15"
        node.id = "21ABC"
        node.state = "ACTIVE"
        
        node.services["AM"]  = [Service(a_type="FDSP_ACCESS_MGR",auto_name="AM")]
        node.services["DM"]  = [Service(a_type="FDSP_DATA_MGR",auto_name="DM")]
        node.services["PM"]  = [Service(a_type="FDSP_PLATFORM",auto_name="PM")]
        node.services["SM"]  = [Service(a_type="FDSP_STOR_MGR",auto_name="SM")]  
        
        nodes = [node]
        
        table = ResponseWriter.prep_node_for_table(self.auth, nodes)              
        row = table[0]
        
        assert row["Name"] == node.name
        assert row["ID"] == node.id
        assert row["State"] == node.state
        assert row["IP V4 Address"] == node.ip_v4_address
        
    def test_prep_services(self):
        '''
        Test that the service preparation is done properly
        '''
        node = Node()
        node.name = "FakeNode"
        node.ip_v4_address = "10.12.14.15"
        node.id = "21ABC"
        node.state = "ACTIVE"
        
        node.services["AM"]  = [Service(a_type="FDSP_ACCESS_MGR",auto_name="AM",status="DOWN",an_id=1,port=7000)]
        node.services["DM"]  = [Service(a_type="FDSP_DATA_MGR",auto_name="DM",status="UP",an_id=2, port=7001)]
        node.services["PM"]  = [Service(a_type="FDSP_PLATFORM",auto_name="PM",status="UP",an_id=3, port=7002)]
        node.services["SM"]  = [Service(a_type="FDSP_STOR_MGR",auto_name="SM",an_id=4, port=7003)]  
        
        nodes = [node]
        
        table = ResponseWriter.prep_services_for_table(self.auth, nodes)
        am = None
        pm = None
        sm = None
        dm = None
        
        for row in table:
            s_type = row["Service Type"]
            
            if (s_type == "AM"):
                am = row
                
            if (s_type == "DM"):
                dm = row
            
            if (s_type == "PM"):
                pm = row
            
            if (s_type == "SM"):
                sm = row
        
        assert am["Service Type"] == "AM"
        assert am["Service ID"] == 1
        assert am["Node Name"] == node.name
        assert am["Node ID"] == node.id
        assert am["Status"] == "DOWN"
        
        assert dm["Service Type"] == "DM"
        assert dm["Service ID"] == 2
        assert dm["Node Name"] == node.name
        assert dm["Node ID"] == node.id
        assert dm["Status"] == "UP"
        
        assert pm["Service Type"] == "PM"
        assert pm["Service ID"] == 3
        assert pm["Node Name"] == node.name
        assert pm["Node ID"] == node.id
        assert pm["Status"] == "UP"
        
        assert sm["Service Type"] == "SM"
        assert sm["Service ID"] == 4
        assert sm["Node Name"] == node.name
        assert sm["Node ID"] == node.id
        assert sm["Status"] == "UNKNOWN"        
        
    def test_prep_snapshot(self):
        '''
        Test that a snapshot gets prepped for the table properly
        '''
        
        snapshot = Snapshot()
        snapshot.id = 100
        snapshot.name = "FirstSnap"
        snapshot.timeline_time = 30000
        snapshot.retention = 0

        t = time.time()
        
        snapshot.created = t
        snapshots = [snapshot]
        
        table = ResponseWriter.prep_snapshot_for_table(self.auth, snapshots)
        row = table[0]
        
        created = time.localtime( snapshot.created )
        created = time.strftime( "%c", created )
        
        assert row["ID"] == snapshot.id
        assert row["Name"] == snapshot.name
        assert row["Created"] == created
        assert row["Retention"] == "Forever"
        
    def test_prep_snapshot_policy(self):
        '''
        Test that snapshot policies are being prepped for table properly
        '''
        snapshot_policy = SnapshotPolicy()
        snapshot_policy.id = 20
        snapshot_policy.name = "TestPolicy"
        snapshot_policy.retention = 0
        snapshot_policy.timeline_time = 0
        
        rule = RecurrenceRule()
        rule.byhour = [3]
        rule.byminute = [30]
        rule.byday = ["SU", "FR"]
        rule.frequency = "MONTHLY"
        
        snapshot_policy.recurrence_rule = rule
        
        policies = [snapshot_policy]
        
        table = ResponseWriter.prep_snapshot_policy_for_table(self.auth, policies)
        row = table[0]
        
        assert row["ID"] == snapshot_policy.id
        assert row["Name"] == snapshot_policy.name
        assert row["Retention"] == "Forever"
        assert row["Recurrence Rule"] == '{"BYMINUTE": [30], "FREQ": "MONTHLY", "BYHOUR": [3], "BYDAY": ["SU", "FR"]}'
        
    def test_prep_domains(self):
        '''
        Test that the domains get prepped properly for the table
        '''
        domain = Domain()
        
        domain.id = 1
        domain.name = "Fremont"
        domain.site = "US"
        
        domains = [domain]
        
        table = ResponseWriter.prep_domains_for_table(self.auth, domains)
        row = table[0]
        
        assert row["ID"] == domain.id
        assert row["Name"] == domain.name
        assert row["Site"] == domain.site
