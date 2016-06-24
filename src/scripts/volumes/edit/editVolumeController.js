angular.module( 'volumes' ).controller( 'editVolumeController', ['$scope', '$volume_api', '$snapshot_service', '$rootScope', '$filter', function( $scope, $volume_api, $snapshot_service, $rootScope, $filter ){
    
    $scope.disableName = true;
    $scope.disableTiering = true;
    $scope.enableDc = false;
    $scope.thisVolume = {};
    $scope.connectorSettings = {};
    
    var initQosSettings = function(){
        $scope.editQos.iopsMin = $scope.thisVolume.qosPolicy.iopsMin;
        $scope.editQos.iopsMax = $scope.thisVolume.qosPolicy.iopsMax;
        $scope.editQos.priority = $scope.thisVolume.qosPolicy.priority;
        $scope.mediaPolicy = $scope.thisVolume.mediaPolicy;
    };
    
    var initSnapshotSettings = function(){

        var realPolicies = $scope.thisVolume.dataProtectionPolicy.snapshotPolicies;

        var notTimelinePolicies = [];
        var timelinePolicies = [];

        for ( var i = 0; i < realPolicies.length; i++ ){
            if ( realPolicies[i].type.indexOf( 'SYSTEM_TIMELINE' ) === -1 ){
                notTimelinePolicies.push( realPolicies[i] );
            }
            else {
                timelinePolicies.push( realPolicies[i] );
            }
        }

        $scope.snapshotPolicies = notTimelinePolicies;
        $scope.timelinePolicies = {
            commitLogRetention: $scope.thisVolume.dataProtectionPolicy.commitLogRetention,
            snapshotPolicies: timelinePolicies
        };
    };
    
    var initConnectorSettings = function(){
        
        $scope.connectorSettings = $scope.thisVolume.settings;
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
            initConnectorSettings();
            
            $scope.$broadcast('fds::fui-slider-refresh' );
            $scope.$broadcast( 'fds::qos-reinit' );
        }
    });
    
    $scope.cancel = function(){
        
        $scope.volumeVars.editing = false;
        $scope.volumeVars.back();
    };
    
    $scope.commitChanges = function(){
        
        $scope.$broadcast( 'fds::refresh' );
        
        $scope.thisVolume.qosPolicy = {
            priority: $scope.editQos.priority,
            iopsMax: $scope.editQos.iopsMax,
            iopsMin: $scope.editQos.iopsMin
        };
        
        $scope.thisVolume.dataProtectionPolicy = $scope.timelinePolicies;
        
        $scope.thisVolume.settings = $scope.connectorSettings;
        
        $volume_api.save( $scope.thisVolume, function( volume ){
            $scope.volumeVars.selectedVolume = volume;
            $scope.cancel();
        });
    };
    
}]);