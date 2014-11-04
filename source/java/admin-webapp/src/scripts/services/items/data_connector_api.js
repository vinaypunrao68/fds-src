angular.module( 'volume-management' ).factory( '$data_connector_api', ['$http', function(){

    var api = {};
    api.connectors = [];

    var getConnectorTypes = function(){
//        return $http.get( '/api/config/data_connectors' )
//            .success( function( data ){
//                api.connectors = data;
//            });

        api.connectors = [{
            type: 'BLOCK',
            api: 'Basic, Cinder',
            options: {
                max_size: '100',
                unit: ['GB', 'TB', 'PB']
            },
            attributes: {
                size: '10',
                unit: 'GB'
            }
        },
        {
            type: 'OBJECT',
            api: 'S3, Swift'
        }];
    }();

    api.editConnector = function( connector ){

        api.connectors.forEach( function( realConnector ){
            if ( connector.type === realConnector.type ){
                realConnector = connector;
            }
        });
    };

    return api;
}]);
