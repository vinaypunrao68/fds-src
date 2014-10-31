angular.module( 'activity-management' ).factory( '$activity_service', [ '$http_fds', function( $http_fds ){

    var service = {};

    service.getActivities = function( filter, callback ){

//        return $http_fds.get( '/scripts/services/data/fakelogs.js',
//            function( response ){
//                callback( eval( response )[0] );
//            });
        
        filter = {
            points: 10
        };
        
        return $http_fds.put( '/api/config/events', filter, callback );
    };

    return service;
}]);
