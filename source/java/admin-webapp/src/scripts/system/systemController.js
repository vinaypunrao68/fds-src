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

    $scope.getStatus = function( state ){

        switch( state ){
            case $node_service.FDS_NODE_UP:
                return 'icon-excellent state-ok';
            case $node_service.FDS_NODE_DOWN:
                return 'icon-issues state-down';
            default:
                return 'icon-nothing state-unknown';
        }
    };

    $scope.getIcon = function( state ){
        return $node_service.classForState( state );
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
