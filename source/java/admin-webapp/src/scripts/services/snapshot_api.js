angular.module( 'qos' ).factory( '$snapshot_service', ['$http', function( $http ){

    var service = {};

    service.createSnapshotPolicy = function( policy, callback, failure ){

        return $http.post( '/api/config/snapshots/policies', policy )
            .success( callback )
            .error( failure );
    };

    service.deleteSnapshotPolicy = function( policy, callback, failure ){

        return $http.delete( '/api/config/snapshot/policies/' + policy.id )
            .success( callback ).error( failure );
    };

    service.attachPolicyToVolue = function( policy, volumeId, callback, failure ){

        return $http.post( '/api/config/snapshots/policies/' + policy.id + '/attach', volumeId )
            .success( callback )
            .error( failure );
    };

    service.detachPolicy = function( policy, volumeId, callback, failure ){

        return $http.put( '/api/config/snapshots/policies/' + policy.id + '/detach', {volumeId: volumeId} )
            .success( callback ).error( failure );
    };

    service.cloneSnapshotToNewVolume = function( snapshot, volumeName, callback, failure ){

        return $http.post( '/api/snapshot/clone/' + snapshot.id + '/' + escape( volumeName ) )
            .success( callback ).error( failure );
    };

    return service;
}]);
