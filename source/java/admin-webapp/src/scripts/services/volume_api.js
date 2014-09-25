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

    api.save = function( volume, callback ){

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

    api.delete = function( volume ){
        return $http.delete( '/api/config/volumes/' + volume.id )
            .success( getVolumes )
            .error( function(){
                alert( 'Volume deletion failed.' );
            });
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
