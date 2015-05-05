angular.module( 'volumes' ).controller( 'volumeCreateController', ['$scope', '$rootScope', '$volume_api', '$snapshot_service', '$modal_data_service', '$http_fds', '$filter', function( $scope, $rootScope, $volume_api, $snapshot_service, $modal_data_service, $http_fds, $filter ){

    $scope.snapshotPolicies = [];
    $scope.dataConnector = {};
    $scope.volumeName = '';
    $scope.mediaPolicy = 0;
    $scope.enableDc = false;
    
    $scope.timelinePolicies = {};
    
    var creationCallback = function( volume, newVolume ){

        //SNAPSHOT SCHEDULES
        
        // for each time deliniation used we need to create a policy and attach
        // it using the volume id in the name so we can identify it easily
        for ( var i = 0; angular.isDefined( volume.snapshotPolicies ) && i < volume.snapshotPolicies.length; i++ ){
            volume.snapshotPolicies[i].name =
                volume.snapshotPolicies[i].name + '_' + newVolume.id;

            $snapshot_service.createSnapshotPolicy( volume.snapshotPolicies[i], function( policy, code ){
                // attach the policy to the volume
                $snapshot_service.attachPolicyToVolume( policy, newVolume.id, function(){} );
            });
        }

        // TIMELINE SCHEDULES
        
        for ( var j = 0; angular.isDefined( volume.timelinePolicies ) && j < volume.timelinePolicies.length; j++ ){
            
            var policy = volume.timelinePolicies[j];
            
            policy.name = newVolume.id + '_TIMELINE_' + policy.recurrenceRule.FREQ;
            
            // create the policy
            $snapshot_service.createSnapshotPolicy( policy, function( policy, rtnCode ){
                // attach the policy
                $snapshot_service.attachPolicyToVolume( policy, newVolume.id, function(){} );
            });
        }
        
        $scope.cancel();
    };
    
    var createVolume = function( volume ){
        
        /**
        *  TODO:  Put real value here
        *  Because the tiering option is not present yet, we will set it to the default here
        *
        **/
//        volume.mediaPolicy = 'HDD_ONLY';
        
        /**
        * Because this is a shim the API does not yet have business
        * logic to combine the attachments so we need to do this in many calls
        * TODO:  Replace with server side logic
        **/
        $volume_api.save( volume, function( newVolume ){ creationCallback( volume, newVolume ); },
            function( response, code ){
                
                $http_fds.genericFailure( response, code );
            
            });
    };
    
    var cloneVolume = function( volume ){
        volume.id = $scope.volumeVars.cloneFromVolume.id;
        
        if ( angular.isDate( volume.timelineTime )){
            volume.timelineTime = volume.timelineTime.getTime();
        }
        
//        if ( angular.isDefined( $scope.volumeVars.clone.cloneType ) ){
//            $volume_api.cloneSnapshot( volume, function( newVolume ){ creationCallback( volume, newVolume ); } );
//        }
//        else {
            $volume_api.clone( volume, function( newVolume ){ creationCallback( volume, newVolume ); } );
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
        volume.sla = $scope.newQos.sla;
        volume.limit = $scope.newQos.limit;
        volume.priority = $scope.newQos.priority;
        volume.snapshotPolicies = $scope.snapshotPolicies;
        volume.timelinePolicies = $scope.timelinePolicies.policies;
        volume.commit_log_retention = $scope.timelinePolicies.commitLogRetention;
        volume.data_connector = $scope.dataConnector;
        volume.name = $scope.volumeName;
        volume.mediaPolicy = $scope.mediaPolicy.value;
        
        if ( !angular.isDefined( volume.name ) || volume.name === '' ){
            
            var $event = {
                text: 'A volume name is required.',
                type: 'ERROR'
            };
            
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
        
        $scope.enableDc = false;
        
        $scope.newQos = {
            sla: volume.sla,
            limit: volume.limit,
            priority: volume.priority
        };
        
        $scope.dataConnector = volume.data_connector;
        
        $scope.$broadcast( 'fds::tiering-choice-refresh' );
        $scope.$broadcast('fds::fui-slider-refresh' );
        $scope.$broadcast( 'fds::qos-reinit' );
    };
    
    $scope.$watch( 'volumeVars.cloneFromVolume', function( newVal, oldVal ){
        
        if ( !angular.isDefined( $scope.volumeVars.toClone) || $scope.volumeVars.toClone.value === 'new' ){
            $scope.enableDc = true;
            return;
        }
        
        if ( !angular.isDefined( newVal ) ){
            $scope.enableDc = true;
            return;
        }
    
        syncWithClone( newVal );
    });
    
    $scope.$watch( 'volumeVars.toClone', function( newVal ){
        
        if ( !angular.isDefined( newVal ) || newVal.value === 'new' ){
            $scope.enableDc = true;
            return;
        }
        
        if ( !angular.isDefined( $scope.volumeVars.cloneFromVolume ) ){
            $scope.enableDc = true;
            return;
        }
        
        syncWithClone( $scope.volumeVars.cloneFromVolume );
    });

    $scope.$watch( 'volumeVars.creating', function( newVal ){
        if ( newVal === true ){
            
            $scope.newQos = {
                sla: 0,
                limit: 0,
                priority: 7
            };
            
            $scope.volumeName = '';
            
            $scope.snapshotPolicies = [];
            
            // default timeline policies
            $scope.timelinePolicies = {
                commitLogRetention: 24*60*60,
                policies: [
                    // daily
                    {
                        retention: 7*24*60*60,
                        recurrenceRule: {FREQ: 'DAILY'}
                    },
                    {
                        retention: 30*24*60*60,
                        recurrenceRule: {FREQ: 'WEEKLY'}
                    },
                    {
                        retention: 180*24*60*60,
                        recurrenceRule: {FREQ: 'MONTHLY'}
                    },
                    {
                        retention: 5*366*24*60*60,
                        recurrenceRule: {FREQ: 'YEARLY'}
                    }
                ]
            };;
            
            $scope.$broadcast( 'fds::tiering-choice-refresh' );
            $scope.$broadcast('fds::fui-slider-refresh' );
            $scope.$broadcast( 'fds::qos-reinit' );
        }
    });

}]);
