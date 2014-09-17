angular.module( 'volume-management' ).factory( '$volume_api', [ '$http', '$rootScope', '$interval', function( $http, $rootScope, $interval ){

    var api = {};

    var pollerId;

    var getVolumes = function(){
        return $http.get( '/api/config/volumes' )
            .success( function( data ){
                api.volumes = data;
            })
            .error( function(){
            });
    };

    var poll = function(){

        getVolumes().then( function(){
            pollerId = $interval( getVolumes, 10000 );
        });
    }();

    $rootScope.$on( 'fds::authentication_logout', function(){
        $interval.cancel( pollerId );
    });

    $rootScope.$on( 'fds::authentication_success', poll );

    api.save = function( volume ){

        // save a new one
        if ( !angular.isDefined( volume.id ) ){
            return $http.post( '/api/config/volumes', volume )
                .success( getVolumes )
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

        return $http.get( '/api/config/volumes/' + volumeId + '/snapshots/policies' )
            .success( callback )
            .error( failure );
    };

    api.volumes = [];

    return api;

}]);
