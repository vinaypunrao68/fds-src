angular.module( 'activity-management' ).factory( '$inbox_service', [ '$http', function( $http ){

    var service = {};

    service.getMessages = function( callback ){

        return $http.get( '/scripts/services/data/fakemessages.js' )
            .success( function( response ){
                callback( response );
            });
    };

    return service;
}]);
