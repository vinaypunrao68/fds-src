angular.module( 'node-management' ).factory( '$node_service', ['$http_fds', '$interval', '$rootScope', function( $http_fds, $interval, $rootScope ){

    var service = {};

    var pollerId;
    service.nodes = [];
    service.detachedNodes = [];

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

        nodes.forEach( function( node ){
            $http_fds.post( '/api/config/services/' + node.node_uuid, {am: node.am, sm: node.sm, dm: node.dm} )
                .then( getNodes );
        });
    };

    var getNodes = function(){
        return $http_fds.get( '/api/config/services',
            function( data ){

                service.nodes = data;
//                parseData( data );
                $interval.cancel( pollerId );
            });
    };

    var poll = function(){

        getNodes().then( function(){
            pollerId = $interval( getNodes, 10000 );
        });
    }();

    $rootScope.$on( 'fds::authentication_logout', function(){
        $interval.cancel( pollerId );
    });

    $rootScope.$on( 'fds::authentication_success', poll );

    return service;

}]);
