mockNode = function(){
    
    angular.module( 'node-management' ).factory( '$node_service', [ function(){

        var service = {};
        
        var pollerId;
        
        var createNode = function( name ){
            
            var node = 
            {
              "uid": 1,
              "name": name,
              "address": {
                "ipv6address": "0.0.0.0",
                "ipv4address": "127.0.0.1"
              },
              "state": "UP",
              "serviceMap": {
                "DM": [
                  {
                    "uid": 7088430947183220035,
                    "name": "DM",
                    "port": 7031,
                    "status": {
                        "state": "RUNNING",
                        "code": 0,
                        "description": ""
                    },
                    "type": "DM"
                  }
                ],
                "AM": [
                  {
                    "uid":7088430947183220035,
                    "name": "AM",
                    "port": 7041,
                    "status": {
                        "state": "DEGRADED",
                        "code": 3001,
                        "description": "Service is degraded"
                    },
                    "type": "AM"
                  }
                ],
                "SM": [
                  {
                    "uid" : 7088430947183220035,
                    "name": "SM",
                    "port": 7021,
                    "status": {
                        "state": "ERROR",
                        "code": 4002,
                        "description": "Service is dead."
                    },
                    "type": "SM"
                  }
                ],
                "PM": [
                  {
                    "uid": 7088430947183220035,
                    "name": "PM",
                    "port": 7001,
                    "status": {
                        "state": "UNREACHABLE",
                        "code": 404,
                        "description": "Cannot reach service."
                    },
                    "type": "PM"
                  }
                ],
                "OM": [
                  {
                    "uid": 7088430947183220035,
                    "name": "OM",
                    "port": 7001,
                    "status": {
                        "state": "RUNNING",
                        "code": 0,
                        "description": "All is good."
                    },
                    "type": "OM"
                  }
                ]
              }
            };
              
            return node;
        };
        
        service.detachedNodes = [
            {
              "name": "awesome-new-node",
              "uid": 2,
              "address": {
                  "ipv6Address": "0.0.0.0",
                  "ipv4Address": "127.0.0.1"
              },
              "state": "DISCOVERED",
              "serviceMap": {
                "PM": [
                  {
                    "uid": 7088430947183220035,
                    "name": "PM",
                    "port": 7031,
                    "status": {
                        "state": "NOT_RUNNING",
                        "code": 0,
                        "description": "Service is down"
                    },
                    "type": "PM"
                  }
                ]
              }
            }
        ];

        service.nodes = [createNode( 'awesome-node' )];

        service.FDS_NODE_UP = 'FDS_Node_Up';
        service.FDS_NODE_DOWN = 'FDS_Node_Down';
        service.FDS_START_MIGRATION = 'FDS_Start_Migration';
        service.FDS_NODE_INACTIVE = 'FDS_Node_Inactive';
        service.FDS_NODE_ATTENTION = 'FDS_Node_Attention';
        service.FDS_NODE_DISCOVERED = 'FDS_Node_Discovered';

        service.FDS_RUNNING = 'RUNNING';
        service.FDS_NOT_RUNNING = 'NOT_RUNNING';
        service.FDS_ERROR = 'ERROR';
        service.FDS_LIMITED = 'LIMITED';
        service.FDS_DEGRADED = 'DEGRADED';
        service.FDS_UNREACHABLE = 'UNREACHABLE';
        service.FDS_UNEXPECTED_EXIT = 'UNEXPECTED_EXIT';
        service.FDS_SHUTTING_DOWN = 'SHUTTING_DOWN';
        service.FDS_INITIALIZING = 'INITIALIZING';

        var saveNodes = function(){
            
            if ( !angular.isDefined( window ) || !angular.isDefined( window.localStorage ) ){
                return;
            }
            
            window.localStorage.setItem( 'nodes', JSON.stringify( service.nodes ) );
            window.localStorage.setItem( 'detachedNodes', JSON.stringify( service.detachedNodes ) );
        };
        
        var init = function(){
            
            if ( window.localStorage.getItem( 'nodes' ) !== null ){
                return;
            }
            
            saveNodes();
        };
        
        init();
        
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
                
                
                nodes[i].serviceMap = {
                    "DM": [
                        {
                            "uid": nodes[i].uid + 1,
                            "name": "DM",
                            "port": 7031,
                            "status": {
                                "state": service.FDS_RUNNING,
                                "code": 0,
                                "description": ""
                            },
                            "type": "DM"
                        }
                    ],
                    "AM": [
                        {
                            "uid": nodes[i].uid + 2,
                            "name": "AM",                                
                            "port": 7041,
                            "status": {
                                "state": service.FDS_RUNNING,
                                "code": 0,
                                "description": ""
                            },
                            "type": "AM"
                        }
                    ],
                    "SM": [
                        {
                            "uid": nodes[i].uid + 3,
                            "name": "SM",
                            "port": 7021,
                            "status": {
                                "state": service.FDS_RUNNING,
                                "code": 0,
                                "description": ""
                            },
                            "type": "SM"
                        }
                    ],
                    "PM": [
                        {
                            "uid": nodes[i].uid + 4,
                            "name": "PM",
                            "port": 7001,
                            "status": {
                                "state": service.FDS_RUNNING,
                                "code": 0,
                                "description": ""
                            },
                            "type": "PM"
                        }
                    ],
                    "OM": [
                        {
                            "uid": nodes[i].uid + 5,
                            "name": "OM",
                            "port": 7001,
                            "status": {
                                "state": service.FDS_RUNNING,
                                "code": 0,
                                "description": ""
                            },
                            "type": "OM"
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

            return service.nodes;
        };
        
        return service;

    }]);
};
