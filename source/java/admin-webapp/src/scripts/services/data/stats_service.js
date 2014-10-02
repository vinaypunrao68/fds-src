angular.module( 'statistics' ).factory( '$stats_service', ['$http', function( $http ){

    var service = {};

    service.getFirebreakSummary = function( callback ){

        return $http.get( '/scripts/services/data/fakefirebreak.js' )
            .success( function( response ){
                callback( eval( response )[0] );
            });
    };

    return service;
}]);
