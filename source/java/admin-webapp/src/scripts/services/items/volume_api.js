angular.module( 'volume-management' ).factory( '$volume_api', [ '$http', '$rootScope', '$interval', function( $http, $rootScope, $interval ){

    var api = {};

    var pollerId;

    var getVolumes = function( callback ){

        return $http.get( '/api/config/volumes' )
            .success( function( data ){
                api.volumes = data;

                if ( angular.isDefined( callback ) && angular.isFunction( callback ) ){
                    callback( api.volumes );
                }
            })
            .error( function(){
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
            return $http.post( '/api/config/volumes', volume )
                .success(
                    function( response ){

                        getVolumes();

                        if ( angular.isDefined( callback ) ){
                            callback( response );
                        }
                    })
                .error( function(){
                    alert( 'Volume creation failed.' );
                });
        }
        // update an existing one
        else {
            return $http.put( '/api/config/volumes/' + volume.id, volume )
                .success( getVolumes );
        }
    };
    
    api.clone = function( volume, callback, failure ){
        return $http.post( '/api/config/volumes/clone/' + volume.id + '/' + volume.name )
            .success(
                function( response ){

                    getVolumes();

                    if ( angular.isDefined( callback ) ){
                        callback( response );
                    }
                })
            .error( function(){
                alert( 'Volume creation failed.' );
            });
    };
    
    api.cloneSnapshot = function( volume, callback, failure ){
        return $http.post( '/api/config/snapshot/clone/' + volume.id + '/' + volume.name )
            .success(
                function( response ){

                    getVolumes();

                    if ( angular.isDefined( callback ) ){
                        callback( response );
                    }
                })
            .error( function(){
                alert( 'Volume creation failed.' );
            });
    };

    api.delete = function( volume ){
        return $http.delete( '/api/config/volumes/' + volume.id )
            .success( getVolumes )
            .error( function(){
                alert( 'Volume deletion failed.' );
            });
    };
    
    api.createSnapshot = function( volumeId, newName, callback, failure ){
        
        return $http.post( '/api/config/volume/snapshot', {volumeId: volumeId, name: newName, retention: 0} )
            .success( callback ).error( failure );
    };
    
    api.deleteSnapshot = function( volumeId, snapshotId, callback, failure ){
        $http.delete( '/api/config/snapshot/' + volumeId + '/' + snapshotId )
            .success( callback ).error( failure );
    };

    api.getSnapshots = function( volumeId, callback, failure ){

        return $http.get( '/api/config/volumes/' + volumeId + '/snapshots' )
            .success( callback ).error( failure );
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

        return $http.get( '/api/config/volumes/' + volumeId + '/snapshot/policies' )
            .success( callback )
            .error( failure );
    };

    api.refresh = function( callback ){
        getVolumes( callback );
    };

    api.volumes = [];

    return api;

}]);
