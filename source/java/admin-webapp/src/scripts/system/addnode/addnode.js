angular.module( 'node-management' ).controller( 'addNodeController', ['$scope','$node_service', '$filter', function( $scope, $node_service, $filter ){

    $scope.checkState = 'partial';
    $scope.addAllNodes = true;
    
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

    var rationalizeParentCheck = function(){
        
        var yesAdd = false;
        var noAdd = false;
        
        for ( var i = 0; i < $scope.detachedNodes.length; i++ ){
            
            var node = $scope.detachedNodes[i];
            
            // this only deals with the addAll variable now 
            // because we aren't allowing service selection. 
            // When that is added we'll need to account for it here.
            if ( node.addAll === true ){
                yesAdd = true;
            }
            else if ( node.addAll === false ){
                noAdd = true;
            }
        }
        
        if ( yesAdd === true && noAdd === false ){
            $scope.addAllNodes = true;
        }
        else if ( yesAdd === false && noAdd === true ){
            $scope.addAllNodes = false;
        }
        else {
            $scope.addAllNodes = 'partial';
        }
    };
    
    $scope.nodeStateChanged = function( detachedNode ){

        detachedNode.am = detachedNode.addAll;
        detachedNode.sm = detachedNode.addAll;
        detachedNode.dm = detachedNode.addAll;
        
        rationalizeParentCheck();
    };
    
    var setNodeList = function( value ){
        $scope.detachedNodes = $node_service.detachedNodes;

        for ( var i = 0; i < $scope.detachedNodes.length; i++ ){
            
            // if not value is provided we will use the state of the column check
            // partial = false
            if ( !angular.isDefined( value ) ){
                
                value = $scope.addAllNodes;
            
                if ( value === 'partial' ){
                    value = false;
                }
            }
                 
            // if either a value was provided, or the node has not gotten an addstate attached we set the value
            if ( angular.isDefined( value ) || !angular.isDefined( $scope.detachedNodes[i].addAll ) ){
                $scope.detachedNodes[i].addAll = value;
                $scope.nodeStateChanged( $scope.detachedNodes[i] );
            }
        }
    };

    $scope.$watch( function(){ return $node_service.detachedNodes; }, function(){
        if ( !$scope.editing ) {
            setNodeList();
        }
    });
    
    $scope.$watch( 'addAllNodes', function( newVal, oldVal ){
        
        if ( newVal === oldVal ){
            return;
        }
        
        if ( newVal === true || newVal === false ){
            
            setNodeList( newVal );
        }
    });
    
    setNodeList( true );
}]);
