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

//    service.classForState = function( state ){
//        switch ( state ){
//            case service.FDS_NODE_DOWN:
//                return 'fui-cross';
//            case service.FDS_NODE_UP:
//                return 'fui-check-inverted-2';
//            case service.FDS_START_MIGRATION:
//                return 'fui-radio-unchecked';
//            case service.FDS_NODE_ATTENTION:
//                return 'fui-chat';
//            case service.FDS_NODE_INACTIVE:
//                return 'fui-radio-unchecked';
//            default:
//                return 'fui-radio-unchecked';
//        }
//    };

    service.addNodes = function( nodes ){

        nodes.forEach( function( node ){
            $http_fds.post( '/api/config/services/' + node.node_uuid, {am: node.am, sm: node.sm, dm: node.dm} )
                .then( getNodes );
        });
    };

//    /**
//    * There is no node reporting service, so data is gathered by getting
//    * a list of services, grouping them by ID and then coallating the data.
//    * TODO:  Fix this.
//    **/
//    var parseData = function( raw ){
//
//        var temp = [];
//        var tempDetached = [];
//        service.detachedNodes = [];
//
//        raw.forEach( function( nodeService ){
//
//            // this will tell us if the node is a detachedNode
//            if ( nodeService.node_type == 'FDSP_PLATFORM' &&
//                nodeService.node_state == service.FDS_NODE_DISCOVERED ){
//                tempDetached.push( nodeService );
//                return;
//            }
//
//            var ref = temp[nodeService.node_uuid];
//
//            if ( !angular.isDefined( ref ) ){
//                ref = nodeService;
//            }
//
//            switch( nodeService.node_type ){
//                case 'FDSP_STOR_MGR':
//                    ref.sm = nodeService.node_state;
//                    break;
//                case 'FDSP_DATA_MGR':
//                    ref.dm = nodeService.node_state;
//                    break;
//                case 'FDSP_STOR_HVISOR':
//                    ref.am = nodeService.node_state;
//                    break;
//                case 'FDSP_PLATFORM':
//                    ref.hw = nodeService.node_state;
//                    break;
//                case 'FDSP_ORCH_MGR':
//                    ref.om = nodeService.node_state;
//                    break;
//            }
//
//            temp[ref.node_uuid] = ref;
//
//        });
//
//        service.detachedNodes = tempDetached;
//
//        var serializedBucket = [];
//
//        for ( var prop in temp ){
//            serializedBucket.push( temp[prop] );
//        }
//
//        service.nodes = serializedBucket;
//    };

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
