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
    
    $scope.creationChoices = [
        { label: $filter( 'translate' )( 'volumes.l_blank' ), value: 'new' },
        { label: $filter( 'translate' )( 'volumes.l_clone' ), value: 'clone' }
    ];
    
    var snapshotsReturned = function( snapshots ){
        
        $scope.ranges = [];
        
        for ( var i = 0; i < snapshots.length; i++ ){
            
            var ms = parseInt( snapshots[i].creationTime.seconds ) * 1000;
            
            var range = {
                min: new Date( ms )
            };
            
            $scope.ranges.push( range );
        }
        
        // create teh continuous range
        $scope.now = (new Date()).getTime();
        $scope.ranges.push( { min: ($scope.now - $scope.volumeVars.cloneFromVolume.dataProtectionPolicy.commitLogRetention.seconds*1000), max: $scope.now, pwidth: 15 } );
        $scope.volumeVars.cloneFromVolume.timelineTime = $scope.now;
    };
    
    var init = function(){
        
        $scope.volumeVars.toClone = $scope.creationChoices[0];        
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
        
        if ( !angular.isDefined( newVal ) || !angular.isDefined( newVal.uid ) ){
            return;
        }
        
        $volume_api.getSnapshots( newVal.uid, snapshotsReturned );
    });
    
    $scope.$watch( 'volumeVars.toClone', function( newVal ){
        
        $scope.volumes = $volume_api.volumes;
        
        if ( angular.isDefined( $scope.volumes ) && $scope.volumes.length > 0 ){
            $scope.volumeVars.cloneFromVolume = $scope.volumes[0];
        }
        
        if ( newVal.value === 'clone' ){
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
            
            $scope.$broadcast( 'fds::timeslider-refresh' );
        }
    });
    
    $scope.$watch( function(){ return $volume_api.volumes; }, function(){

        if ( !$scope.volumeVars.toClone.value !== 'clone' ) {
            $scope.volumes = $volume_api.volumes;
        }
    });
    
    $scope.$watch( 'volumeVars.creating', refresh );
    
    init();
    
}]);