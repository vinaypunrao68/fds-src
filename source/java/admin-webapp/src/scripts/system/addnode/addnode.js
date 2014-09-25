angular.module( 'node-management' ).controller( 'addNodeController', ['$scope','$node_service', function( $scope, $node_service ){

    $scope.checkState = 'partial';

    $scope.detachedNodes = [];

//    var isChecked = function( val ){
//
//        if ( val === 'checked' ){
//            return true;
//        }
//        else {
//            return false;
//        }
//    };

    $scope.cancel = function(){
        $scope.$emit( 'fds::node_done_adding' );
    };

    $scope.addNodes = function(){

        var toAdd = [];

        $scope.detachedNodes.forEach( function( node ){

//            node.am = isChecked( node.am );
//            node.dm = isChecked( node.dm );
//            node.sm = isChecked( node.sm );

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
