angular.module( 'volumes' ).controller( 'editVolumeController', ['$scope', '$volume_api', '$snapshot_service', '$rootScope', '$filter', function( $scope, $volume_api, $snapshot_service, $rootScope, $filter ){
    
    $scope.disableName = true;
    $scope.disableTiering = true;
    $scope.thisVolume = {};
    
    var initQosSettings = function(){
        $scope.editQos.capacity = $scope.thisVolume.qosPolicy.iops_min;
        $scope.editQos.limit = $scope.thisVolume.qosPolicy.iops_max;
        $scope.editQos.priority = $scope.thisVolume.qosPolicy.priority;
        $scope.mediaPolicy = $scope.thisVolume.mediaPolicy;
    };
    
    var initSnapshotSettings = function(){

        var realPolicies = $scope.thisVolume.dataProtectionPolicysnapshotPolicies;

        var notTimelinePolicies = [];
        var timelinePolicies = [];

        for ( var i = 0; i < realPolicies.length; i++ ){
            if ( realPolicies[i].id.name.indexOf( '_TIMELINE_' ) === -1 ){
                notTimelinePolicies.push( realPolicies[i] );
            }
            else {
                timelinePolicies.push( realPolicies[i] );
            }
        }

        $scope.snapshotPolicies = notTimelinePolicies;
        $scope.timelinePolicies = {
            commitLogRetention: $scope.thisVolume.commit_log_retention,
            policies: timelinePolicies
        };
    };
    
    $scope.deleteVolume = function(){
        
        var confirm = {
            type: 'CONFIRM',
            text: $filter( 'translate' )( 'volumes.desc_confirm_delete' ),
            confirm: function( result ){
                if ( result === false ){
                    return;
                }
                
                $volume_api.delete( $scope.volumeVars.selectedVolume,
                    function(){ 
                        var $event = {
                            type: 'INFO',
                            text: $filter( 'translate' )( 'volumes.desc_volume_deleted' )
                        };

                        $rootScope.$emit( 'fds::alert', $event );
                        $scope.volumeVars.back();
                        $scope.volumeVars.back();
                });
            }
        };
        
        $rootScope.$emit( 'fds::confirm', confirm );
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
        
        $scope.thisVolume.qosPolicy = {
            priority: $scope.editQos.priority,
            iops_max: $scope.editQos.limit,
            iops_min: $scope.editQos.sla
        };
        
        $scope.thisVolume.commitLogRetention = $scope.timelinePolicies.commitLogRetention;
        
        $volume_api.save( $scope.thisVolume, function( volume ){
//            $snapshot_service.saveSnapshotPolicies( $scope.thisVolume.id, $scope.timelinePolicies.policies );
            $scope.volumeVars.selectedVolume = volume;
            $scope.cancel();
        });
    };
    
}]);