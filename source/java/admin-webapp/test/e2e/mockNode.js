mockNode = function(){
    
    angular.module( 'node-management' ).factory( '$node_service', [ function(){

        var service = {};

        var pollerId;
        service.nodes = [];
        service.detachedNodes = [];

        service.nodes = {
          "uuid": 1,
          "domain": "fds",
          "nodes": [
            {
              "name": "awesome-node",
              "uuid": 7088430947183220032,
              "ipV6address": "0.0.0.0",
              "ipV4address": "127.0.0.1",
              "state": "UP",
              "services": {
                "DM": [
                  {
                    "uuid": 7088430947183220035,
                    "autoName": "DM",
                    "port": 7031,
                    "status": "INVALID",
                    "type": "FDSP_DATA_MGR"
                  }
                ],
                "AM": [
                  {
                    "uuid": 7088430947183220035,
                    "autoName": "AM",
                    "port": 7041,
                    "status": "ACTIVE",
                    "type": "FDSP_STOR_HVISOR"
                  }
                ],
                "SM": [
                  {
                    "uuid": 7088430947183220035,
                    "autoName": "SM",
                    "port": 7021,
                    "status": "ERROR",
                    "type": "FDSP_STOR_MGR"
                  }
                ],
                "PM": [
                  {
                    "uuid": 7088430947183220035,
                    "autoName": "PM",
                    "port": 7001,
                    "status": "INACTIVE",
                    "type": "FDSP_PLATFORM"
                  }
                ]
              }
            }
          ]
        };

        service.FDS_NODE_UP = 'FDS_Node_Up';
        service.FDS_NODE_DOWN = 'FDS_Node_Down';
        service.FDS_START_MIGRATION = 'FDS_Start_Migration';
        service.FDS_NODE_INACTIVE = 'FDS_Node_Inactive';
        service.FDS_NODE_ATTENTION = 'FDS_Node_Attention';
        service.FDS_NODE_DISCOVERED = 'FDS_Node_Discovered';

        service.FDS_ACTIVE = 'ACTIVE';
        service.FDS_INACTIVE = 'INACTIVE';
        service.FDS_ERROR = 'ERROR';

        service.getOverallStatus = function( node ){

            if ( node.am === service.FDS_NODE_DOWN ||
                node.om === service.FDS_NODE_DOWN ||
                node.sm === service.FDS_NODE_DOWN ||
                node.hw === service.FDS_NODE_DOWN ||
                node.dm === service.FDS_NODE_DOWN ){
                return service.FDS_NODE_DOWN;
            }

            if ( node.am === service.FDS_NODE_ATTENTION ||
                node.om === service.FDS_NODE_ATTENTION ||
                node.sm === service.FDS_NODE_ATTENTION ||
                node.hw === service.FDS_NODE_ATTENTION ||
                node.dm === service.FDS_NODE_ATTENTION ){
                return service.FDS_NODE_ATTENTION;
            }

            return service.FDS_NODE_UP;
        };

        service.addNodes = function( nodes ){

        };

        var getNodes = function(){

        };
        
        return service;

    }]);
};
