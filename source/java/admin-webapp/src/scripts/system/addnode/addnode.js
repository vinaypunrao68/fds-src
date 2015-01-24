angular.module( 'node-management' ).controller( 'addNodeController', ['$scope','$node_service', '$filter', function( $scope, $node_service, $filter ){

    $scope.checkState = 'partial';
    
    $scope.onText = ($filter( 'translate' )( 'common.l_on' )).toUpperCase();
    $scope.offText = ($filter( 'translate' )( 'common.l_off' )).toUpperCase();

    $scope.detachedNodes = [];

    $scope.cancel = function(){
        $scope.systemVars.back();
    };

    $scope.addNodes = function(){

        var toAdd = [];

        $scope.detachedNodes.forEach( function( node ){

            if ( node.addAll === true || node.addAll === 'partial' ){
                toAdd.push( node );
            }
        });

        $node_service.addNodes( toAdd );

        $scope.cancel();
    };

    $scope.parentStateChanged = function( detachedNode ){

        detachedNode.am = detachedNode.addAll;
        detachedNode.sm = detachedNode.addAll;
        detachedNode.dm = detachedNode.addAll;
    };

    $scope.serviceStateChanged = function( detachedNode ){

        if ( detachedNode.am === true &&
            detachedNode.sm === true &&
            detachedNode.dm === true ){

            detachedNode.addAll = 'checked';
        }
        else if ( (detachedNode.am === false || !angular.isDefined( detachedNode.am ) ) &&
                 ( detachedNode.dm === false || !angular.isDefined( detachedNode.dm ) ) &&
                 (detachedNode.sm === false || !angular.isDefined( detachedNode.sm ) ) ){
            detachedNode.addAll = false;
        }
        else {
            detachedNode.addAll = 'partial';
        }
    };

    $scope.$watch( function(){ return $node_service.detachedNodes; }, function(){
        if ( !$scope.editing ) {
            $scope.detachedNodes = $node_service.detachedNodes;
        }
    });
}]);
