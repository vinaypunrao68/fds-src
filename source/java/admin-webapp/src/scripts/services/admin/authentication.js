angular.module( 'user-management' ).factory( '$authentication', ['$http', '$document', '$rootScope', '$authorization', function( $http, $document, $rootScope, $authorization ){

    var service = {};
    service.isAuthenticated = ($document[0].cookie !== '');
    service.error = undefined;

    var clearCookie = function() {
        $document[0].cookie = 'token=; expires=Thu, 01-Jan-70 00:00:01 GMT;';
        $document[0].cookie = 'user=; expires=Thu, 01-Jan-70 00:00:01 GMT;';
    };

    service.login = function( username, password ){

        $http.post( '/api/auth/token?login=' + username + '&' + 'password=' + password, {} )
            .success( function( response ){
                service.error = '';
                $document[0].cookie = 'token=' + response.token;
                $document[0].cookie = 'user=' + JSON.stringify( response );
                service.isAuthenticated = true;
                $rootScope.$broadcast( 'fds::authentication_success' );
                $authorization.setUser( response );
            })
            .error( function( response, code ){
                service.error = code + ':' + response.message + ' - Please try again';
                clearCookie();
                $rootScope.$broadcast( 'fds::authentication_failure' );
            });
    };

    service.logout = function() {
        clearCookie();
        service.error = undefined;
        service.isAuthenticated = false;
        $rootScope.$broadcast( 'fds::authentication_logout' );
    };

    service.hasError = function(){
        if ( angular.isDefined( service.error ) && service.error !== '' ){
            return true;
        }

        return false;
    };

    return service;
}]).config( ['$httpProvider', function( $httpProvider ){

    $httpProvider.interceptors.push( function( $q ){

        return {
            responseError: function( config ){
                console.log( 'Error: ' + config.status );
                if ( config.status === 401 && this.document.cookie !== '' ){
                    this.document.cookie = 'token=; expires=Thu, 01-Jan-70 00:00:01 GMT;';
                    this.document.cookie = 'user=; expires=Thu, 01-Jan-70 00:00:01 GMT;';
                    location.reload();
                    return $q.reject( config );
                }

                return config;
            }
        };
    });

}]);
