angular.module( 'user-management' ).factory( '$user_service', [ '$http', function( $http ){

    var service = {};

    service.getUsers = function( callback ){

        return $http.get( '/api/system/users' )
            .success( function( response ){
                callback( response );
            });
    };

    service.createUser = function( username, password, success, failure ){
        return $http.post( '/api/system/users/' + username + '/' + password )
            .success( success ).error( failure );
    };

    service.changePassword = function( userId, newPassword, success, failure ){

        return $http.put( '/api/system/users/' + userId + '/' + newPassword )
            .success( success ).error( failure );

    };

    return service;
}]);
