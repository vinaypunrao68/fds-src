angular.module( 'system' ).controller( 'systemController', [ '$scope', '$node_service', '$authentication', '$state', '$timeout', '$filter', '$rootScope', '$byte_converter', function( $scope, $node_service, $authentication, $state, $timeout, $filter, $rootScope, $byte_converter ){

    $scope.addingnode = false;
    $scope.nodes = $node_service.nodes;
    
    $scope.sortPredicate = 'node_name';

    $scope.addNode = function(){
        $scope.systemVars.next( 'addNode' );
    };
    
    $scope.removeNode = function( node ){
        var confirm = {
            type: 'CONFIRM',
            text: $filter( 'translate' )( 'system.desc_remove_node' ),
            confirm: function( result ){
                if ( result === false ){
                    return;
                }
                
                $node_service.removeNode( node,
                    function(){ 
                        var $event = {
                            type: 'INFO',
                            text: $filter( 'translate' )( 'system.desc_node_removed' )
                        };

                        $rootScope.$emit( 'fds::alert', $event );
                    });
            }
        };
        
        $rootScope.$emit( 'fds::confirm', confirm );
        
    };
    
    $scope.getOverallStatus = function( node ){
        return $scope.getStatus( $node_service.getOverallStatus( node ) );
    };

    $scope.getStatus = function( service ){
        
        if ( !angular.isDefined( service ) ){
            return 'icon-nothing state-unknown';
        }
        
        if ( service.length > 0 ){
            service = service[0];
        }
        
        switch( service.status.state ){
            case $node_service.FDS_RUNNING:
                return 'icon-excellent state-ok';
            case $node_service.FDS_LIMITED:
            case $node_service.FDS_DEGRADED:
            case $node_service.FDS_INITIALIZING:
            case $node_service.FDS_SHUTTING_DOWN:
                return 'icon-issues state-issues';
            case $node_service.FDS_ERROR:
            case $node_service.FDS_UNEXPECTED_EXIT:
                return 'icon-warning state-down';
            case $node_service.FDS_UNREACHABLE:
            default:
                return 'icon-nothing state-unknown';
        }
    };
    
    $scope.getByteString = function( value, units ){
        
        return $byte_converter.convertFromUnitsToString( value, units );
    };

    $scope.$on( 'fds::node_done_adding', function(){
        $scope.addingnode = false;
    });
    
    $scope.$on( 'fds::authentication_success', function(){
        $timeout( $state.reload );
        console.log( 'nodes calling refresh' );
        $node_service.refresh();
    });

    $scope.$watch( function(){ return $node_service.nodes; }, function(){

        if ( !$scope.addingnode ) {
            $scope.nodes = $node_service.nodes;
        }
    });
    
    $node_service.refresh();
}]);
