angular.module( 'volumes' ).controller( 'viewVolumeController', ['$scope', '$volume_api', '$modal_data_service', '$snapshot_service', function( $scope, $volume_api, $modal_data_service, $snapshot_service ){
    
    $scope.snapshots = [];
    
    $scope.clone = function( snapshot ){

        var newName = prompt( 'Name for new volume:' );
        
        $snapshot_service.cloneSnapshotToNewVolume( snapshot, newName.trim(), function(){ alert( 'Clone successfully created.');} );
    };
    
    $scope.deleteSnapshot = function( snapshot ){
        
        $volume_api.deleteSnapshot( $scope.volumeVars.selectedVolume.id, snapshot.id, function(){ alert( 'Snapshot deleted successfully.' );} );
    };

    $scope.formatDate = function( ms ){
        var d = new Date( parseInt( ms ) );
        return d.toString();
    };

    // when we get shown, get all the snapshots and policies.  THen do the chugging
    // to display the summary and set the hidden forms.
    $scope.$watch( 'volumeVars.viewing', function( newVal ){

        if ( newVal === true ){
            $volume_api.getSnapshots( $scope.volumeVars.selectedVolume.id, function( data ){ $scope.snapshots = data; } );
        }
    });
    
}]);
