angular.module( 'volumes' ).controller( 'volumeCreateController', ['$scope', '$rootScope', '$volume_api', '$snapshot_service', '$modal_data_service', '$http_fds', '$filter', function( $scope, $rootScope, $volume_api, $snapshot_service, $modal_data_service, $http_fds, $filter ){

    $scope.snapshotPolicies = [];
    $scope.dataSettings = {};
    $scope.volumeName = '';
    $scope.mediaPolicy = 1;
    $scope.enableDc = false;
    $scope.volumeNameRequired = true;
    $scope.enableType = false;
    $scope.enableSize = false;
    
    $scope.timelinePolicies = {};

    var createVolume = function( volume ){

        $volume_api.save( volume, function( newVolume ){ 
            
            // this basically just goes back to the list page
            $scope.cancel();
        });
    };
    
    var cloneVolume = function( volume ){
        volume.uid = $scope.volumeVars.cloneFromVolume.uid;
        
        if ( angular.isDate( volume.timelineTime )){
            volume.timelineTime = volume.timelineTime.getTime();
        }
        
//        if ( angular.isDefined( $scope.volumeVars.clone.cloneType ) ){
//            $volume_api.cloneSnapshot( volume, function( newVolume ){ creationCallback( volume, newVolume ); } );
//        }
//        else {
            $volume_api.clone( volume, function( newVolume ){ 
                // this basically just goes back to the list page
                $scope.cancel();
            });
//        }
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
                        $scope.cancel();
                });
            }
        };
        
        $rootScope.$emit( 'fds::confirm', confirm );
    };
    
    $scope.save = function(){
        
        $scope.$broadcast( 'fds::refresh' );
        
        var volume = {};
        volume.name = $scope.volumeName;
        
        volume.qosPolicy = {
            iopsMin: $scope.newQos.iopsMin,
            iopsMax: $scope.newQos.iopsMax,
            priority: $scope.newQos.priority
        };
        
        volume.dataProtectionPolicy = {
            snapshotPolicies: $scope.timelinePolicies.snapshotPolicies,
            commitLogRetention: $scope.timelinePolicies.commitLogRetention
        };
        
        volume.settings = $scope.dataSettings;
        
        volume.mediaPolicy = $scope.mediaPolicy.value;
        
        // make sure a name is present
        if ( !angular.isDefined( volume.name ) || volume.name === '' ){
            
            var $event = {
                text: 'A volume name is required.',
                type: 'ERROR'
            };
            
            $rootScope.$emit( 'fds::alert', $event );
            return;
        }
        
        // make sure a name has no spaces
        if ( volume.name.indexOf( ' ' ) !== -1 ){
            
            var $event = {
                text: 'Volume names must not contain spaces.',
                type: 'ERROR'
            };
            
            $rootScope.$emit( 'fds::alert', $event );
            return;
        }
        
        // make sure for iSCSI the password has correct length
        if ( volume.settings.type == 'ISCSI' && volume.settings.target.incomingUsers.length > 0 ){
            
            if ( volume.settings.target.incomingUsers[0].password.length > 0 &&
                (volume.settings.target.incomingUsers[0].password.length < 12 ||
                 volume.settings.target.incomingUsers[0].password.length > 16 ) ){
            
                var $event = {
                    text: 'iSCSI password must be between 12 and 16 characters.',
                    type: 'ERROR'
                };

                $rootScope.$emit( 'fds::alert', $event );
                return;
            }
        }
        
        // make sure if it's NFS that an IP filter is set.
        if ( volume.settings.type == 'NFS' && volume.settings.clients.length == 0 ){
            
            var $event = {
                text: 'The \'Allowed IP Filters\' field must not be empty.  It will be reset to \'*\'.',
                type: 'ERROR'
            };
            
            $scope.dataSettings.clients = '*';
            $rootScope.$emit( 'fds::alert', $event );
            return;
        }
        
        if ( angular.isDefined( $scope.volumeVars.toClone ) && $scope.volumeVars.toClone.value === 'clone' ){
            volume.timelineTime = $scope.volumeVars.cloneFromVolume.timelineTime;
            cloneVolume( volume );
        }
        else{
            createVolume( volume );
        }

    };

    $scope.cancel = function(){
        $scope.volumeVars.creating = false;
        $scope.volumeVars.toClone = {value: false};
        $scope.volumeVars.back();
    };
    
    var syncWithClone = function( volume ){
        
        $scope.enableType = false;
        $scope.enableSize = true;
        
        $scope.newQos = {
            iopsMin: volume.qosPolicy.iopsMin,
            iopsMax: volume.qosPolicy.iopsMax,
            priority: volume.qosPolicy.priority
        };
        
        $scope.timelinePolicies = volume.dataProtectionPolicy;
        
        $scope.dataSettings = volume.settings;
        
        $scope.$broadcast( 'fds::tiering-choice-refresh' );
        $scope.$broadcast('fds::fui-slider-refresh' );
        $scope.$broadcast( 'fds::qos-reinit' );
    };
    
    $scope.$watch( 'volumeVars.cloneFromVolume', function( newVal, oldVal ){
        
        if ( !angular.isDefined( $scope.volumeVars.toClone) || $scope.volumeVars.toClone.value === 'new' ){
            $scope.enableType = true;
            $scope.enableSize = true;
            return;
        }
        
        if ( !angular.isDefined( newVal ) ){
            $scope.enableType = true;
            $scope.enableSize = true;
            return;
        }
    
        syncWithClone( newVal );
    });
    
    $scope.$watch( 'volumeVars.toClone', function( newVal ){
        
        if ( !angular.isDefined( newVal ) || newVal.value === 'new' ){
            $scope.enableType = true;
            $scope.enableSize = true;
            return;
        }
        
        if ( !angular.isDefined( $scope.volumeVars.cloneFromVolume ) ){
            $scope.enableType = true;
            $scope.enableSize = true;
            return;
        }
        
        syncWithClone( $scope.volumeVars.cloneFromVolume );
    });

    $scope.$watch( 'volumeVars.creating', function( newVal ){
        if ( newVal === true ){
            
            $scope.newQos = {
                iopsMin: 0,
                iopsMax: 0,
                priority: 7
            };
            
            $scope.volumeName = '';
            
            $scope.snapshotPolicies = [];
            
            $scope.$broadcast( 'fds::timeline_init' );
            $scope.$broadcast( 'fds::tiering-choice-refresh' );
            $scope.$broadcast('fds::fui-slider-refresh' );
            $scope.$broadcast( 'fds::qos-reinit' );
        }
    });

}]);
