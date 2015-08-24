angular.module( 'volumes' ).controller( 'volumeController', [ '$scope', '$location', '$state', '$volume_api', '$rootScope', '$filter', '$element', '$authorization', '$timeout', '$byte_converter', function( $scope, $location, $state, $volume_api, $rootScope, $filter, $element, $authorization, $timeout, $byte_converter ){
    
    $scope.searchText = '';
    $scope.sortPredicate = '';
    $scope.reverse = true;
    
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
    
    $scope.customSorter = function( volume ){
        
        var rtnValue = '';
        var alteredPredicate = $scope.sortPredicate;
        
        if ( $scope.sortPredicate.indexOf( '-' ) === 0 ){
            $scope.reverse = true;
            alteredPredicate = $scope.sortPredicate.substr( 1 );
        }
        else {
            $scope.reverse = false;
        }
        
        switch( $scope.sortPredicate ){
            case 'size':
                var num = 0;
                unit = 'B';
                
                if ( angular.isDefined( volume.current_usage ) ){
                    
                    if ( angular.isDefined( volume.current_usage.size ) ){
                        num = parseInt( volume.current_usage.size );
                    }
                    
                    if ( angular.isDefined( volume.current_usage.unit ) ){
                        unit = volume.current_usage.unit;
                    }
                }
                
                if ( unit === 'KB' ){
                    num *= 1024;
                }
                else if ( unit === 'MB' ){
                    num *= Math.pow( 1024, 2 );
                }
                else if ( unit === 'GB' ){
                    num *= Math.pow( 1024, 3 );
                }
                else if ( unit === 'TB' ){
                    num *= Math.pow( 1024, 4 );
                }
                else if ( unit === 'PB' ){
                    num *= Math.pow( 1024, 5 );
                }
                else if ( unit === 'EB' ){
                    num *= Math.pow( 1024, 6 );
                }
                
                rtnValue = num;
            case 'firebreak':
                break;
            default:
                rtnValue = volume[ alteredPredicate ];
        }
        
        return rtnValue;
    };
    
    $scope.getFirebreakColor = function( volume ){
        
        var getColor = function( num ){
            
            if ( num <= 3600 ){
                return '#FF5D00';
            }
            else if ( num <= 3600*3 ){
                return '#FD8D00';
            }
            else if ( num <= 3600*6 ){
                return '#FCE300';
            }
            else if ( num <= 3600*12 ){
                return '#C0DF00';
            }
            else if ( num <= 3600*24 ){
                return '#68C000';
            }
            else {
                return 'rgba( 255, 255, 255, 0.0)';
            }
        };

        var now = new Date();
        var capacityFirebreak = 3600*25;
        var performanceFirebreak = capacityFirebreak;
        
        if ( angular.isDefined( volume.firebreak ) ){
            
            now.getTime() - volume.status.lastCapacityFirebreak.seconds*1000;
            now.getTime() - volume.status.lastPerformanceFirebreak.seconds*1000;
        }
        
        if ( capacityFirebreak < performanceFirebreak ){
            return getColor( capacityFirebreak );
        }
        else {
            return getColor( performanceFirebreak );
        }
        
    };
    
    $scope.getCapacityString = function( usage ){
        
        return $byte_converter.convertBytesToString( usage.value );
    };
    
    $scope.$on( 'fds::authentication_logout', function(){
        $scope.volumes = [];
    });
    
    $scope.$on( 'fds::authentication_success', function(){
        $timeout( $state.reload );
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
            $scope.volumeVars.editing = false;
        }
    });
    
    $scope.$watch( 'volumeVars.creating', function( newVal ){ if ( newVal === false ){ $volume_api.refresh(); }} );

    $scope.isAllowed = $authorization.isAllowed;
    $volume_api.refresh();
}]);
