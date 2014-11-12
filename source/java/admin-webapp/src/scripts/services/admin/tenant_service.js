angular.module( 'tenant-management' ).factory( '$tenant_service', ['$http_fds', function( $http_fds ){

    var service = {};

    service.getTenants = function( callback, failure ){

        return $http_fds.get( '/api/system/tenants', callback, failure );
    };

    service.createTenant = function( tenant, callback, failure ){

        return $http_fds.post( '/api/system/tenants/' + tenant.name, {}, callback, failure );
    };

    service.attachUser = function( tenant, userId, callback, failure ){

        return $http_fds.put( '/api/system/tenants/' + tenant.id + '/' + userId, {}, callback, failure );
    };
    
    service.revokeUser = function( tenant, userId, callback, failure ){
        
        return $http_fds.put( '/api/system/tenants/revoke/' + tenant.id + '/' + userId, {}, callback, failure );
    };

    return service;

}]);
