angular.module( 'user-management' ).factory( '$domain_service', ['$http', function(){

    var service = {};

    service.getDomains = function(){

        if ( !angular.isDefined( service.domains ) ){
//            $http.get( '/api/config/domains' )
//                .success( function( data ){
//                    service.domains = data;
//                })
//                .then( function(){ return service.domains; });
            service.domains = [{fqdn: 'com.goldman', name: 'FDS Global'}];
        }

        return service.domains;
    };

    return service;
}]);
