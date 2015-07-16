angular.module( 'user-management' ).factory( '$user_service', [ '$http_fds', function( $http_fds ){

    var service = {};

    service.getUsers = function( callback ){

        return $http_fds.get( webPrefix + '/users', callback );
    };

    service.createUser = function( user, success, failure ){
        return $http_fds.post( webPrefix + '/users', user, success, failure );
    };

    service.changePassword = function( user, success, failure ){

        return $http_fds.put( webPrefix + '/users/' + user.userId, user, success, failure );

    };

    return service;
}]);
