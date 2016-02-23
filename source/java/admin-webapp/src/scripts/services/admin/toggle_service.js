angular.module( 'system' ).factory( '$toggle_service', [ '$http_fds', '$q', function( $http_fds, $q ){

    var service = {};
    
    var toggles = undefined;
    
    service.STATS_QUERY_TOGGLE = 'fds.feature_toggle.om.enable_stats_service_query';

    service.getToggles = function( callback ){
        
        var deferred = $q.defer();
        
        if ( !angular.isDefined( toggles ) ){
            
            service.loadToggles( function( rtn ){
                deferred.resolve( rtn );
            });
        }
        else {
            deferred.resolve( toggles );
        }
        
        return deferred.promise;
    };
    
    service.loadToggles = function( callback ){
        
         $http_fds.get( webPrefix + '/capabilities', function( capabilities ){
             
             toggles = capabilities.toggles;
             
             if ( angular.isDefined( callback ) && angular.isFunction( callback ) ){
                 callback( toggles );
             }
         });
    };
    
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
