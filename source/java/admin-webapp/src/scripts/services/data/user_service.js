angular.module( 'user-management' ).factory( '$user_service', [ '$http', function( $http ){

    var service = {};

    service.getUsers = function( callback ){

        return $http.get( '/scripts/services/data/fakeusers.js' )
            .success( function( response ){
                callback( response );
            });
    };

    service.changePassword = function( userId, newPassword, success, failure ){

        return $http.put( '/api/system/users/' + userId + '/' + newPassword )
            .success( success ).error( failure );

    };

    return service;
}]);
