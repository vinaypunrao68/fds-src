angular.module( 'volumes' ).controller( 'volumeController', [ '$scope', '$location', '$state', '$volume_api', '$rootScope', '$filter', '$element', '$authorization', '$timeout', '$byte_converter', function( $scope, $location, $state, $volume_api, $rootScope, $filter, $element, $authorization, $timeout, $byte_converter ){
    
    $scope.searchText = '';
    $scope.sortPredicate = '';
    $scope.reverse = true;
    
    $scope.volumesBySize = [];
    $scope.volumesByType = [];
    
    $scope.clicked = function( volume){
        $scope.volumeVars.selectedVolume = volume;
        $scope.volumeVars.viewing = true;
        $scope.volumeVars.next( 'viewvolume' );
    };
    
    var sumTheSizes = function(){
        
        var nfs_count = 0;
        var iscsi_count = 0;
        var object_count = 0;
        var nfs_size = 0;
        var iscsi_size = 0;
        var object_size = 0;
        
        for( var i = 0; i < $scope.volumes.length; i++ ){
            var volume = $scope.volumes[i];
            var usage = volume.status.currentUsage.value;
            
            if ( volume.settings.type === 'NFS' ){
                nfs_count++;
                nfs_size += usage;
            }
            else if ( volume.settings.type === 'ISCSI' ){
                iscsi_count++;
                iscsi_size += usage;
            }
            else if ( volume.settings.type === 'OBJECT' ){
                object_count++;
                object_size += usage;
            }
        }
        
        var totalSize = nfs_size + iscsi_size + object_size;
        
        var buildPercentage = function( size, total ){
            var perc = ( size / total * 100 ).toFixed( 0 );
            
            if ( isNaN( perc ) ){
                perc = 0;
            }
            
            return perc;
        };
        
        $scope.volumesBySize = [
            { name: 'NFS', value: nfs_size, printable: $byte_converter.convertBytesToString( nfs_size ), percentage: buildPercentage( nfs_size, totalSize ) },
            { name: 'iSCSI', value: iscsi_size, printable: $byte_converter.convertBytesToString( iscsi_size ), percentage: buildPercentage( iscsi_size, totalSize ) },
            { name: 'Object', value: object_size, printable: $byte_converter.convertBytesToString( object_size ), percentage: buildPercentage( object_size, totalSize ) }
        ];
        
        $scope.volumesByType = [
            { name: 'NFS', value: nfs_count, percentage: buildPercentage( nfs_count, $scope.volumes.length ) },
            { name: 'iSCSI', value: iscsi_count, percentage: buildPercentage( iscsi_count, $scope.volumes.length ) },
            { name: 'Object', value: object_count, percentage: buildPercentage( object_count, $scope.volumes.length ) }
        ];
    };
    
    $scope.sizeTooltip = function( entry ){
        return entry.name + ': ' + entry.percentage + '% (' + entry.printable + ')';
    };
    
    $scope.countTooltip = function( entry ){
        var str = entry.name + ': ' + entry.percentage + '% (' + entry.value + ' volumes)';
        return str;
    };
    
    $scope.pieColors = function( entry ){
        
        var color = 'black';
        
        if ( !angular.isDefined( entry.name ) ){
            return color;
        }
        
        if ( entry.name.toLowerCase() === 'nfs' ){
            color = '#4857C4';
        }
        else if ( entry.name.toLowerCase() === 'iscsi' ){
            color = '#8784DE';
        }
        else if ( entry.name.toLowerCase() === 'object' ){
            color = '#489AE1';
        }
        
        return color;
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
                
                if ( angular.isDefined( volume.status.currentUsage ) ){
                    
                    if ( angular.isDefined( volume.status.currentUsage.size ) ){
                        num = parseInt( volume.status.currentUsage.size );
                    }
                    
                    if ( angular.isDefined( volume.status.currentUsage.unit ) ){
                        unit = volume.status.currentUsage.unit;
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
    
    $scope.getCapacityString = function( volume ){
        
        var usage = volume.status.currentUsage;
        var usedStr = $byte_converter.convertBytesToString( usage.value );
        var allocated = '';
        
        if ( angular.isDefined( volume.settings.capacity ) ){
            allocated = ' / ' + $byte_converter.convertBytesToString( volume.settings.capacity.value ); 
        }
        
        return usedStr + allocated;
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
            
            // put together the data for our pie charts.
            sumTheSizes();
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
