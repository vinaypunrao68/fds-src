angular.module( 'volumes' ).controller( 'cloneVolumeController', ['$scope', '$volume_api', '$filter', function( $scope, $volume_api, $filter ){
    
    $scope.volumeVars.cloneFromVolume = undefined;
    $scope.volumes = $volume_api.volumes;
    $scope.ranges = [];
    $scope.cloneOptions = [];
    $scope.now = (new Date()).getTime();
    
    $scope.domainLabels = [];
    
    $scope.dateLabelFunction = function( value ){
        var date = new Date( value );
        return date.toLocaleDateString() + ' ' + date.getHours() + ':' + date.getMinutes();
//        return value;
    };
    
    var snapshotsReturned = function( snapshots ){
        
        $scope.ranges = [];
        
        for ( var i = 0; i < snapshots.length; i++ ){
            
            var range = {
                min: new Date( snapshots[i].creation )
            };
            
            $scope.ranges.push( range );
        }
        
        // create teh continuous range
        $scope.now = (new Date()).getTime();
        $scope.ranges.push( { min: ($scope.now - $scope.volumeVars.cloneFromVolume.commit_log_retention*1000), max: $scope.now, pwidth: 15 } );
        $scope.volumeVars.cloneFromVolume.timelineTime = $scope.now;
    };
    
    var init = function(){
        
        $scope.volumeVars.toClone = 'new';        
        $scope.choosing = false;
    };
    
    var refresh = function( newValue ){
        
        if ( newValue === false ){
            return;
        }
        
        $scope.cloneOptions = $volume_api.volumes;
        $scope.selectedItem = $scope.cloneOptions[0];
        
        init();
    };
    
    // combo changed - need to refresh the timeline
    $scope.$watch( 'volumeVars.cloneFromVolume', function( newVal ){
        
        if ( !angular.isDefined( newVal ) || !angular.isDefined( newVal.id ) ){
            return;
        }
        
        $volume_api.getSnapshots( newVal.id, snapshotsReturned );
    });
    
    $scope.$watch( 'volumeVars.toClone', function( newVal ){
        
        $scope.volumes = $volume_api.volumes;
        
        if ( angular.isDefined( $scope.volumes ) && $scope.volumes.length > 1 ){
            $scope.volumeVars.cloneFromVolume = $scope.volumes[0];
        }
        
        if ( newVal === 'clone' ){
            $scope.now = (new Date()).getTime();
            
            $scope.domainLabels = [
                { text: $filter( 'translate' )( 'common.l_now' ), value: $scope.now },
                { text: $filter( 'translate' )( 'common.l_1_day' ), value: $scope.now - (24*1000*60*60) },
                { text: $filter( 'translate' )( 'common.l_30_days' ), value: $scope.now - (24*1000*60*60*30)},
                { text: $filter( 'translate' )( 'common.l_1_week' ), value: $scope.now - (24*1000*60*60*7) },
                { text: '1 ' + $filter( 'translate' )( 'common.l_year' ), value: $scope.now - (24*1000*60*60*366) },
                { text: '5 ' + $filter( 'translate' )( 'common.l_year' ), value: $scope.now - (24*1000*60*60*366*5) },
                { text: '10 ' + $filter( 'translate' )( 'common.l_year' ), value: $scope.now - (24*1000*60*60*366*10) }
            ];
            
            $scope.volumeVars.cloneFromVolume.timelineTime = $scope.now;
        }
    });
    
    $scope.$watch( function(){ return $volume_api.volumes; }, function(){

        if ( !$scope.volumeVars.toClone !== 'clone' ) {
            $scope.volumes = $volume_api.volumes;
        }
    });
    
    $scope.$watch( 'volumeVars.creating', refresh );
    
    init();
    
}]);