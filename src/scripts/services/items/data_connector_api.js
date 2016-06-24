angular.module( 'volume-management' ).factory( '$data_connector_api', ['$http_fds', function( $http_fds ){

    var api = {};

    api.getVolumeTypes = function( callback ){
        return $http_fds.get( webPrefix + '/volume_types' )
            .success( function( data ){
                api.types = data;
            
                if ( angular.isFunction( callback ) ){
                    callback( api.types );
                }
            });
    };

    return api;
}]);
