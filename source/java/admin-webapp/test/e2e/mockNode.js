mockNode = function(){
    
    angular.module( 'node-management' ).factory( '$node_service', [ function(){

        var service = {};
        
        var pollerId;
        
        service.detachedNodes = [
            {
              "name": "awesome-new-node",
              "uuid": 2,
              "ipV6address": "0.0.0.0",
              "ipV4address": "127.0.0.1",
              "state": "DISCOVERED",
              "services": {
                "DM": [
                  {
                    "uuid": 7088430947183220035,
                    "autoName": "PM",
                    "port": 7031,
                    "status": "INVALID",
                    "type": "FDSP_PLATFORM"
                  }
                ]
              }
            }
        ];

        service.nodes = [
            {
              "name": "awesome-node",
              "uuid": 1,
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
                ],
                "OM": [
                  {
                    "uuid": 7088430947183220035,
                    "autoName": "OM",
                    "port": 7001,
                    "status": "ACTIVE",
                    "type": "FDSP_ORCH_MGR"
                  }
                ]
              }
            }
          ];

        service.FDS_NODE_UP = 'FDS_Node_Up';
        service.FDS_NODE_DOWN = 'FDS_Node_Down';
        service.FDS_START_MIGRATION = 'FDS_Start_Migration';
        service.FDS_NODE_INACTIVE = 'FDS_Node_Inactive';
        service.FDS_NODE_ATTENTION = 'FDS_Node_Attention';
        service.FDS_NODE_DISCOVERED = 'FDS_Node_Discovered';

        service.FDS_ACTIVE = 'ACTIVE';
        service.FDS_INACTIVE = 'INACTIVE';
        service.FDS_ERROR = 'ERROR';

        var saveNodes = function(){
            
            if ( !angular.isDefined( window ) || !angular.isDefined( window.localStorage ) ){
                return;
            }
            
            if ( window.localStorage.getItem( 'nodes' ) !== null ){
                return;
            }
            
            window.localStorage.setItem( 'nodes', JSON.stringify( service.nodes ) );
            window.localStorage.setItem( 'detachedNodes', JSON.stringify( service.detachedNodes ) );
        };
        
        saveNodes();
        
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

            for ( var i = 0; i < nodes.length; i++ ){
                nodes[i].state = 'UP';
                
                
                nodes[i].services = {
                    "DM": [
                        {
                            "uuid": nodes[i].uuid + 1,
                            "autoName": "DM",
                            "port": 7031,
                            "status": "ACTIVE",
                            "type": "FDSP_DATA_MGR"
                        }
                    ],
                    "AM": [
                        {
                            "uuid": nodes[i].uuid + 2,
                            "autoName": "AM",
                            "port": 7041,
                            "status": "ACTIVE",
                            "type": "FDSP_STOR_HVISOR"
                        }
                    ],
                    "SM": [
                        {
                            "uuid": nodes[i].uuid + 3,
                            "autoName": "SM",
                            "port": 7021,
                            "status": "ACTIVE",
                            "type": "FDSP_STOR_MGR"
                        }
                    ],
                    "PM": [
                        {
                            "uuid": nodes[i].uuid + 4,
                            "autoName": "PM",
                            "port": 7001,
                            "status": "ACTIVE",
                            "type": "FDSP_PLATFORM"
                        }
                    ],
                    "OM": [
                        {
                            "uuid": nodes[i].uuid + 5,
                            "autoName": "OM",
                            "port": 7001,
                            "status": "ACTIVE",
                            "type": "FDSP_ORCH_MGR"
                        }
                    ]
                };
                
                
                service.nodes.push( nodes[i] );
                
                for ( var j = 0; j < service.detachedNodes.length; j++ ){
                    
                    var dNode = service.detachedNodes[j];
                    
                    if ( nodes[i].name === dNode.name ){
                        service.detachedNodes.splice( j, 1 );
                        break;
                    }
                }
                
                saveNodes();
            }
            
        };
        
        service.removeNode = function( node, callback ){
            
            node.state = 'DISCOVERED';
            service.detachedNodes.push( node );
            
            for ( var i = 0; i < service.nodes.length; i++ ){
                
                if ( service.nodes[i].name === node.name ){
                    service.nodes.splice( i, 1 );
                    break;
                }
            }
            
            if ( angular.isFunction( callback ) ){
                callback();
            }
            
            saveNodes();
        };
        
        service.refresh = function(){
            
            if ( angular.isDefined( window ) && angular.isDefined( window.localStorage ) ){
                service.nodes = [];
                service.detachedNodes = [];
                
                var n = JSON.parse( window.localStorage.getItem( 'nodes' ) );
                var d = JSON.parse( window.localStorage.getItem( 'detachedNodes' ) );
                
                if ( n !== null && angular.isDefined( n ) ){
                    service.nodes = n;
                }
                
                if ( d !== null && angular.isDefined( d ) ){
                    service.detachedNodes = d;
                }
            }
        };

        var getNodes = function(){

        };
        
        return service;

    }]);
};
