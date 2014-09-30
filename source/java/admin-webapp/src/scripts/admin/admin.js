angular.module( 'admin-settings' ).controller( 'adminController', ['$scope', '$authentication', '$authorization', '$data_connector_api', function( $scope, $authentication, $authorization, $data_connector_api ){

    $scope.sizes = [{name:'GB'}, {name:'TB'},{name:'PB'}];
    $scope.levels = [{name:'Debug'},{name:'Info'},{name:'Warning'},{name:'Error'},{name:'Critical'}];
    $scope.connectors = $data_connector_api.connectors;
    $scope.alertLevel = $scope.levels[3];
    $scope._selectedSize = 10;
    $scope._selectedUnit = $scope.sizes[0].name;

    var findUnit = function( str ){
        for ( var i = 0; i < $scope.sizes.length; i++ ){
            if ( $scope.sizes[i].name == str ){
                return $scope.sizes[i];
            }
        }
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
        $scope.connectors = $data_connector_api.connectors;
    };

    $scope.isAllowed = $authorization.isAllowed;

}]);
