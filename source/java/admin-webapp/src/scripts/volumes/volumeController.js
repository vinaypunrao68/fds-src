angular.module( 'volumes' ).controller( 'volumeController', [ '$scope', '$location', '$state', '$volume_api', '$rootScope', '$filter', '$element', function( $scope, $location, $state, $volume_api, $rootScope, $filter, $element ){
    
    $scope.clicked = function( volume){
        $scope.volumeVars.selectedVolume = volume;
        $scope.volumeVars.viewing = true;
        $scope.volumeVars.next( 'viewvolume' );
    };
    
//    $scope.takeSnapshot = function( $event, volume ){
//        
//        $event.stopPropagation();
//        
//        var confirm = {
//            type: 'CONFIRM',
//            text: $filter( 'translate' )( 'volumes.desc_confirm_snapshot' ),
//            confirm: function( result ){
//                if ( result === false ){
//                    return;
//                }
//                
//                $volume_api.createSnapshot( volume.id, volume.name + '.' + (new Date()).getTime(), 
//                    function(){ 
//                        var $event = {
//                            type: 'INFO',
//                            text: $filter( 'translate' )( 'volumes.desc_snapshot_created' )
//                        };
//
//                        $rootScope.$emit( 'fds::alert', $event );
//                });
//            }
//        };
//        
//        $rootScope.$emit( 'fds::confirm', confirm );
//    };

    $scope.createNewVolume = function(){
        $scope.volumeVars.creating = true;
        $scope.volumeVars.next( 'createvolume' );
    };
    
    $scope.$on( 'fds::authentication_logout', function(){
        $scope.volumes = [];
    });

    $scope.$watch( function(){ return $volume_api.volumes; }, function(){

        if ( !$scope.editing ) {
            $scope.volumes = $volume_api.volumes;
        }
    });
    
    $scope.$watch( 'volumeVars.index', function( newVal ){
        
        if ( newVal === 0 ){
            $scope.volumeVars.viewing = false;
            $scope.volumeVars.creating = false;
        }
    });
    
    $scope.$watch( 'volumeVars.creating', function( newVal ){ if ( newVal === false ){ $volume_api.refresh(); }} );

    $volume_api.refresh();

}]);
