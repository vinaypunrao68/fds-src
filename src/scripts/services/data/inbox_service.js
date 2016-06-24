angular.module( 'activity-management' ).factory( '$inbox_service', [ '$http_fds', function( $http_fds ){

    var service = {};

    service.getMessages = function( callback ){

        return $http_fds.get( '/scripts/services/data/fakemessages.js',
            function( response ){
                callback( response );
            });
    };

    return service;
}]);
