angular.module( 'volumes' ).controller( 'nameTypeController', ['$scope', '$data_connector_api', '$modal_data_service', function( $scope, $data_connector_api, $modal_data_service ){

    $scope.sizes = [{name:'GB'}, {name:'TB'},{name:'PB'}];
    $scope._selectedSize = 10;
    $scope._selectedUnit = $scope.sizes[0].name;

    var init = function(){
        $scope.name = '';
        $scope.editing = false;
        $scope.connectors = $data_connector_api.connectors;
        $scope.data_connector = $scope.connectors[0];
    };

    var findUnit = function( str ){

        for ( var i = 0; i < $scope.sizes.length; i++ ){
            if ( $scope.sizes[i].name == str ){
                return $scope.sizes[i];
            }
        }
    };

    $scope.setSelected = function( connector ){
        $scope.data_connector = connector;
    };

    $scope.editConnector = function( connector ){
        $scope._selectedUnit = findUnit( connector.attributes.unit );
        $scope._selectedSize = parseInt( connector.attributes.size );
        $scope.editingConnector = connector;

    };

    $scope.stopEditing = function(){
        $scope.editingConnector = {};
    };

    $scope.saveConnectorChanges = function( connector ){
        connector.attributes.size = $scope._selectedSize;
        connector.attributes.unit = $scope._selectedUnit.name;
        $data_connector_api.editConnector( connector );
        $scope.stopEditing();
    };

    $scope.updateData = function(){
        $modal_data_service.update({
            name: $scope.name,
            data_connector: $scope.data_connector
        });
    };

    $scope.$on( 'fds::volume_done_editing', init );

    $scope.$on( 'fds::create_volume_initialize', function(){
        $scope.updateData();
    });

    $scope.$watch( 'name', function(){
        $scope.updateData();
    });

    $scope.$watch( 'data_connector', function(){
        $scope.updateData();
    });

    init();
}]);
