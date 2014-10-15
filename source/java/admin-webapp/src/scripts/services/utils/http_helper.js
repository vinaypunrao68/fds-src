angular.module( 'base' ).factory( '$http_fds', ['$http', function( $http ){
    
    var service = {};
    
    var handleSuccess = function( response, success ){
        
        if ( angular.isFunction( success ) ){
            success( response );
        }
    };
    
    var genericFailure = function( response, code ){
        
        if ( code === 401 || code === 0 ){
            return;
        }
        
        alert( 'Error ' + code + ':\n' + response.message );
    };
    
    var handleError = function( response, code, failure ){
        
        if ( angular.isFunction( failure ) ){
            failure( response, code );
        }
        else if ( angular.isString( failure ) ){
            genericFailure( failure );
        }
        else {
            genericFailure( response, code );
        }
    };
    
    service.post = function( url, data, success, failure ){
        return $http.post( url, data )
            .success( function( response ){
                handleSuccess( response, success );
            })
            .error( function( response, code ){
                handleError( response, code, failure );
            });
    };
    
    service.put = function( url, data, success, failure ){
        return $http.put( url, data )
            .success( function( response ){
                handleSuccess( response, success );
            })
            .error( function( response, code ){
                handleError( response, code, failure );
            });
    };
    
    service.get = function( url, success, failure ){
        return $http.get( url )
            .success( function( response ){
                handleSuccess( response, success );
            })
            .error( function( response, code ){
                handleError( response, code, failure );
            });
    };
    
    service.delete = function( url, success, failure ){
        return $http.delete( url )
            .success( function( response ){
                handleSuccess( response, success );
            })
            .error( function( response, code ){
                handleError( response, code, failure );
            });
    };
    
    return service;
}]);