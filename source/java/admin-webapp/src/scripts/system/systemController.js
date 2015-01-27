angular.module( 'system' ).controller( 'systemController', [ '$scope', '$node_service', '$authentication', function( $scope, $node_service, $authentication ){

    $scope.addingnode = false;
    $scope.nodes = $node_service.nodes;
    
    $scope.sortPredicate = 'node_name';

    $scope.addNode = function(){
        $scope.systemVars.next( 'addNode' );
    };
    
    $scope.getOverallStatus = function( node ){
        return $scope.getStatus( $node_service.getOverallStatus( node ) );
    };

    $scope.getStatus = function( service ){
        
        if ( service.length > 0 ){
            service = service[0];
        }
        
        switch( service.status ){
            case $node_service.FDS_ACTIVE:
                return 'icon-excellent state-ok';
            case $node_service.FDS_INACTIVE:
                return 'icon-issues state-issues';
            case $node_service.FDS_ERROR:
                return 'icon-warning state-down';
            default:
                return 'icon-nothing state-unknown';
        }
    };

    $scope.$on( 'fds::node_done_adding', function(){
        $scope.addingnode = false;
    });

    $scope.$watch( function(){ return $node_service.nodes; }, function(){

        if ( !$scope.addingnode ) {
            $scope.nodes = $node_service.nodes;
        }
    });
}]);
