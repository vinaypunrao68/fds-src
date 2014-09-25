angular.module( 'activity-management' ).factory( '$activity_service', [ '$http', function( $http ){

    var service = {};

    service.getActivities = function( start, end, max, callback ){

        return $http.get( '/scripts/services/data/fakelogs.js' )
            .success( function( response ){
                callback( response );
            });
    };

    return service;
}]);
