angular.module( 'node-management' ).factory( '$node_service', ['$http_fds', '$interval', '$rootScope', function( $http_fds, $interval, $rootScope ){

    var service = {};

    var pollerId;
    service.nodes = [];
    service.detachedNodes = [];

    service.UP = 'UP';
    service.DOWN = 'DOWN';
    service.MIGRATION = 'MIGRATION';
    service.REMOVED = 'REMOVED';
    service.UNKNOWN = 'UNKNOWN';
    service.DISCOVERED = 'DISCOVERED';
    
    service.FDS_ACTIVE = 'ACTIVE';
    service.FDS_INACTIVE = 'INACTIVE';
    service.FDS_ERROR = 'ERROR';

    service.getOverallStatus = function( node ){

        if ( node.am === service.DOWN ||
            node.om === service.DOWN ||
            node.sm === service.DOWN ||
            node.hw === service.DOWN ||
            node.dm === service.DOWN ){
            return service.DOWN;
        }

        if ( node.am === service.UNKNOWN ||
            node.om === service.UNKNOWN ||
            node.sm === service.UNKNOWN ||
            node.hw === service.UNKNOWN ||
            node.dm === service.UNKNOWN ){
            return service.FDS_NODE_ATTENTION;
        }

        return service.UP;
    };

    service.addNodes = function( nodes ){

        nodes.forEach( function( node ){
            $http_fds.post( '/api/config/services/' + node.uuid, {am: node.am, sm: node.sm, dm: node.dm} )
                .then( getNodes );
//            console.log( '/api/config/services/' + node.uuid + ' BODY: {am: ' + node.am + ', sm:' + node.sm + ', dm: ' + node.dm + '}' );
        });
    };
    
    service.removeNode = function( node ){
        
        // right now we stop all services when we deactivate a node
        $http_fds.put( '/api/config/services/' + node.uuid, { am: false, sm: false, dm: false } ).then( getNodes );
    };

    var getNodes = function(){
        
        if ( pollerId === -1 ){
            poll();
        }
        
        return $http_fds.get( '/api/config/services',
            function( data ){

                service.nodes = [];
                service.detachedNodes = [];
                
                // separate out the discovered nodes from the active ones
                for( var i = 0; angular.isDefined( data.nodes ) && i < data.nodes.length; i++ ){
                    
                    var node = data.nodes[i];
                    
                    if ( node.state === service.DISCOVERED ){
                        service.detachedNodes.push( node );
                    }
                    else {
                        service.nodes.push( node );
                    }
                }
            });
    };
    
    service.refresh = function(){
        getNodes();
    };

    var poll = function(){
        pollerId = $interval( getNodes, 30000 );
    }();    
    
    $rootScope.$on( 'fds::authentication_logout', function(){
        console.log( 'node poller cancelled' );
        $interval.cancel( pollerId );
    });

    $rootScope.$on( 'fds::authentication_success', function(){
        service.refresh();
    });

    return service;

}]);
