angular.module( 'volumes' ).controller( 'volumeCreateController', ['$scope', '$volume_api', '$snapshot_service', '$modal_data_service', function( $scope, $volume_api, $snapshot_api, $modal_data_service ){

    $scope.save = function(){

        var volume = $modal_data_service.getData();

        if ( volume.sla === 'None' ){
            volume.sla = 0;
        }

        if ( volume.limit === 'None' ){
            volume.limit = 0;
        }

        /**
        * Because this is a shim the API does not yet have business
        * logic to combine the attachments so we need to do this in many calls
        * TODO:  Replace with server side logic
        **/
        $volume_api.save( volume, function( newVolume ){

                // for each time deliniation used we need to create a policy and attach
                // it using the volume id in the name so we can identify it easily
                for ( var i = 0; angular.isDefined( volume.snapshotPolicies ) && i < volume.snapshotPolicies.length; i++ ){
                    volume.snapshotPolicies[i].name =
                        newVolume.id + '_' + volume.snapshotPolicies[i].name;

                    $snapshot_api.createSnapshotPolicy( volume.snapshotPolicies[i], function( policy, code ){
                        // attach the policy to the volume
                        $snapshot_api.attachPolicyToVolume( policy, newVolume.id, function(){} );
                    });
                }
        });

        $scope.cancel();
    };

    $scope.cancel = function(){
        $scope.$broadcast( 'fds::volume_done_editing' );
        $scope.volumeVars.back();
    };

    $scope.$watch( '$parent.creating', function(){
        if ( $scope.$parent.creating === true ){
            $modal_data_service.start();
            $scope.$broadcast( 'fds::create_volume_initialize' );
        }
    });

}]);
