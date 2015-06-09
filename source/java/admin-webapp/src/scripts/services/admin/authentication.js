angular.module( 'user-management' ).factory( '$authentication', ['$http', '$document', '$rootScope', '$authorization', function( $http, $document, $rootScope, $authorization ){

    var service = {};
    service.isAuthenticated = ($document[0].cookie.indexOf( 'token=' ) !== -1 && $document[0].cookie.indexOf( 'user=' ) !== -1 );
    service.error = undefined;

    var clearCookie = function() {
        $document[0].cookie = 'token=; expires=Thu, 01-Jan-70 00:00:01 GMT;';
        $document[0].cookie = 'user=; expires=Thu, 01-Jan-70 00:00:01 GMT;';
    };

    service.login = function( username, password ){

        var errorFunction = function( response, code ){
            service.error = code + ':' + response.message + ' - Please try again';
            clearCookie();
            $rootScope.$broadcast( 'fds::authentication_failure' );
        };
        
        $http.post( webPrefix + '/token?login=' + username + '&' + 'password=' + password, {} )
            .success( function( response, code ){
                
                if ( code === 401 ){
                    errorFunction( response, code );
                    return;
                }
                
                service.error = '';
                $document[0].cookie = 'token=' + response.token;
                $document[0].cookie = 'user=' + JSON.stringify( response );
                service.isAuthenticated = true;
                $rootScope.$broadcast( 'fds::authentication_success' );
                $authorization.setUser( response );
            })
            .error( errorFunction );
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
            responseError: function( response, error, code ){
                
                var userCookie = readCookie( 'user' );
                var tokenCookie = readCookie( 'token' );
                
                var logout = false;
                
                if ( userCookie !== null || tokenCookie !== null ){
                    if ( response.status === 401 ){
                        logout = true;
                    }

                    if ( response.data !== null && !(response.data instanceof Object) && response.data.indexOf( '{' ) !== -1 ){
                        try{
                            var j_str = response.data.substring( response.data.indexOf( '{' ) );
                            var j_data = JSON.parse( j_str );

                            if ( j_data.code && j_data.code == 'ECONNRESET' ){
                                logout = true;
                            }
                        }
                        catch( o ){
                        }
                    }                    
                }
                
                if ( logout === true ){
                    this.document.cookie = 'token=; expires=Thu, 01-Jan-70 00:00:01 GMT;';
                    this.document.cookie = 'user=; expires=Thu, 01-Jan-70 00:00:01 GMT;';
                    location.reload();
                }
                else {
//                    alert( 'Error ' + response.status + ': ' + response.statusText );
                }
                
                return $q.reject( response );
            }
        };
    });

}]);
