angular.module( 'user-management' ).factory( '$user_service', [ '$http_fds', function( $http_fds ){

    var service = {};

    service.getUsers = function( callback ){

        return $http_fds.get( '/api/system/users', callback );
    };

    service.createUser = function( username, password, success, failure ){
        return $http_fds.post( '/api/system/users/' + username + '/' + password, {}, success, failure );
    };

    service.changePassword = function( userId, newPassword, success, failure ){

        return $http_fds.put( '/api/system/users/' + userId + '/' + newPassword, {}, success, failure );

    };

    return service;
}]);
