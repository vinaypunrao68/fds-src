angular.module( 'volumes' ).controller( 'volumeCreateController', ['$scope', '$rootScope', '$volume_api', '$snapshot_service', '$modal_data_service', '$http_fds', '$filter', function( $scope, $rootScope, $volume_api, $snapshot_service, $modal_data_service, $http_fds, $filter ){

    $scope.qos = {
        capacity: 0,
        limit: 0,
        priority: 10
    };
    
    $scope.snapshotPolicies = [];
    $scope.dataConnector = {};
    $scope.volumeName = '';
    $scope.fake = 'What?';
    
    var creationCallback = function( volume, newVolume ){

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

        
        $scope.cancel();
    };
    
    var createVolume = function( volume ){
        
        /**
        *  TODO:  Put real value here
        *  Because the tiering option is not present yet, we will set it to the default here
        *
        **/
        volume.mediaPolicy = 'HDD_ONLY';
        
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
        volume.id = $scope.volumeVars.clone.id;
        
        if ( angular.isDefined( $scope.volumeVars.clone.cloneType ) ){
            $volume_api.cloneSnapshot( volume, function( newVolume ){ creationCallback( volume, newVolume ); } );
        }
        else {
            $volume_api.clone( volume, function( newVolume ){ creationCallback( volume, newVolume ); } );
        }
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

        var volume = {};
        volume.sla = $scope.qos.capacity;
        volume.limit = $scope.qos.limit;
        volume.priority = $scope.qos.priority;
        volume.snapshotPolicies = $scope.snapshotPolicies;
        volume.data_connector = $scope.dataConnector;
        volume.name = $scope.volumeName;
        
        
        if ( !angular.isDefined( volume.name ) || volume.name === '' ){
            
            var $event = {
                text: 'A volume name is required.',
                type: 'ERROR'
            };
            
            $rootScope.$emit( 'fds::alert', $event );
            return;
        }
        
        if ( $scope.volumeVars.toClone === 'clone' ){
            cloneVolume( volume );
        }
        else{
            createVolume( volume );
        }

    };

    $scope.cancel = function(){
        $scope.volumeVars.creating = false;
        $scope.volumeVars.toClone = false;
        $scope.volumeVars.back();
    };

    $scope.$watch( 'volumeVars.creating', function( newVal ){
        if ( newVal === true ){
            
            $scope.qos = {
                capacity: 0,
                limit: 0,
                priority: 10
            };
            
            $scope.volumeName = '';
            
            $scope.snapshotPolicies = [];
        }
    });

}]);
