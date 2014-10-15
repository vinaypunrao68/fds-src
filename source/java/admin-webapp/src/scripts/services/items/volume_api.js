angular.module( 'volume-management' ).factory( '$volume_api', [ '$http_fds', '$rootScope', '$interval', function( $http_fds, $rootScope, $interval ){

    var api = {};

    var pollerId;

    var getVolumes = function( callback ){

        return $http_fds.get( '/api/config/volumes',
            function( data ){
                api.volumes = data;

                if ( angular.isDefined( callback ) && angular.isFunction( callback ) ){
                    callback( api.volumes );
                }
            });
    };

    $rootScope.$on( 'fds::authentication_logout', function(){
        $interval.cancel( pollerId );
        api.volume = [];
    });

    $rootScope.$on( 'fds::authentication_success', function(){

        getVolumes().then( function(){
            pollerId = $interval( getVolumes, 10000 );
        });
    });

    api.save = function( volume, callback, failure ){

        // save a new one
        if ( !angular.isDefined( volume.id ) ){
            return $http_fds.post( '/api/config/volumes', volume,
                function( response ){

                    getVolumes();

                    if ( angular.isDefined( callback ) ){
                        callback( response );
                    }
                },
                failure );
        }
        // update an existing one
        else {
            return $http_fds.put( '/api/config/volumes/' + volume.id, volume, getVolumes );
        }
    };
    
    api.clone = function( volume, callback, failure ){
        return $http_fds.post( '/api/config/volumes/clone/' + volume.id + '/' + volume.name, volume,
            function( response ){

                getVolumes();

                if ( angular.isDefined( callback ) ){
                    callback( response );
                }
            },
            failure );
    };
    
    api.cloneSnapshot = function( volume, callback, failure ){
        return $http_fds.post( '/api/config/snapshot/clone/' + volume.id + '/' + volume.name, volume,
            function( response ){

                getVolumes();

                if ( angular.isDefined( callback ) ){
                    callback( response );
                }
            },
            failure );
    };

    api.delete = function( volume ){
        return $http_fds.delete( '/api/config/volumes/' + volume.id, getVolumes,
            'Volume deletion failed.' );
    };
    
    api.createSnapshot = function( volumeId, newName, callback, failure ){
        
        return $http_fds.post( '/api/config/volume/snapshot', {volumeId: volumeId, name: newName, retention: 0}, callback, failure );
    };
    
    api.deleteSnapshot = function( volumeId, snapshotId, callback, failure ){
        $http_fds.delete( '/api/config/snapshot/' + volumeId + '/' + snapshotId, callback, failure );
    };

    api.getSnapshots = function( volumeId, callback, failure ){

        return $http_fds.get( '/api/config/volumes/' + volumeId + '/snapshots', callback, failure );
    };

    api.getSnapshotPoliciesForVolume = function( volumeId, callback, failure ){

//        return [{
//            policyName: 'Fake_name',
//            recurrenceRule: {
//                freq: 'YEARLY',
//                count: 5
//            },
//            retentionTime: 3885906383
//        }];

        return $http_fds.get( '/api/config/volumes/' + volumeId + '/snapshot/policies', callback, failure );
    };

    api.refresh = function( callback ){
        getVolumes( callback );
    };

    api.volumes = [];

    return api;

}]);
