angular.module( 'tenant-management' ).factory( '$tenant_service', ['$http_fds', function( $http_fds ){

    var service = {};

    service.getTenants = function( callback, failure ){

        return $http_fds.get( webPrefix + '/tenants', callback, failure );
    };

    service.createTenant = function( tenant, callback, failure ){

        return $http_fds.post( webPrefix + '/tenants', tenant, callback, failure );
    };

    service.attachUser = function( tenant, userId, callback, failure ){

        return $http_fds.post( webPrefix + '/tenants/' + tenant.id + '/' + userId, {}, callback, failure );
    };
    
    service.revokeUser = function( tenant, userId, callback, failure ){
        
        return $http_fds.delete( webPrefix + '/tenants/' + tenant.id + '/' + userId, {}, callback, failure );
    };

    return service;

}]);
