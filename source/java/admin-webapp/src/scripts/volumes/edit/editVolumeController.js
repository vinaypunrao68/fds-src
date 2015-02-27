angular.module( 'volumes' ).controller( 'editVolumeController', ['$scope', '$volume_api', '$snapshot_service', function( $scope, $volume_api, $snapshot_service ){
    
    $scope.disableName = true;
    $scope.disableTiering = true;
    $scope.thisVolume = {};
    
    var initQosSettings = function(){
        $scope.editQos.capacity = $scope.thisVolume.sla;
        $scope.editQos.limit = $scope.thisVolume.limit;
        $scope.editQos.priority = $scope.thisVolume.priority;
        $scope.mediaPolicy = $scope.thisVolume.mediaPolicy;
    };
    
    var initSnapshotSettings = function(){
        $volume_api.getSnapshotPoliciesForVolume( $scope.volumeVars.selectedVolume.id, function( realPolicies ){

            var notTimelinePolicies = [];
            var timelinePolicies = [];

            for ( var i = 0; i < realPolicies.length; i++ ){
                if ( realPolicies[i].name.indexOf( '_TIMELINE_' ) === -1 ){
                    notTimelinePolicies.push( realPolicies[i] );
                }
                else {
                    timelinePolicies.push( realPolicies[i] );
                }
            }

            $scope.snapshotPolicies = notTimelinePolicies;
            $scope.timelinePolicies = {
                continuous: $scope.thisVolume.commit_log_retention,
                policies: timelinePolicies
            };
        });
    };
    
    $scope.$watch( 'volumeVars.editing', function( newVal, oldVal ){
        
        if ( newVal === true ){
            $scope.thisVolume = $scope.volumeVars.selectedVolume;
            initQosSettings();
            initSnapshotSettings();
            
            $scope.$broadcast('fds::fui-slider-refresh' );
            $scope.$broadcast( 'fds::qos-reinit' );
        }
    });
    
    $scope.cancel = function(){
        
        $scope.volumeVars.editing = false;
        $scope.volumeVars.back();
    };
    
    $scope.commitChanges = function(){
        
        $scope.thisVolume.sla = $scope.editQos.sla;
        $scope.thisVolume.priority = $scope.editQos.priority;
        $scope.thisVolume.limit = $scope.editQos.limit;
        
        $scope.thisVolume.commit_log_retention = $scope.timelinePolicies.continuous;
        
        $volume_api.save( $scope.thisVolume, function( volume ){
            $snapshot_service.saveSnapshotPolicies( $scope.thisVolume.id, $scope.timelinePolicies.policies );
            $scope.volumeVars.selectedVolume = volume;
            $scope.cancel();
        });
    };
    
}]);