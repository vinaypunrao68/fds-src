angular.module( 'volumes' ).controller( 'nameTypeController', ['$scope', '$data_connector_api', '$modal_data_service', function( $scope, $data_connector_api, $modal_data_service ){

    $scope.sizes = [{name:'GB'}, {name:'TB'},{name:'PB'}];
    $scope._selectedSize = 10;
    $scope._selectedUnit = $scope.sizes[0];

    $scope.updateData = function(){
        $modal_data_service.update({
            name: $scope.name,
            data_connector: $scope.data_connector
        });
    };
    
    var init = function(){
        $scope.name = '';
        $scope.editing = false;
        $scope.connectors = $data_connector_api.connectors;
        $scope.data_connector = $scope.connectors[1];
        
        $scope.nameEnablement = true;      
        $scope.dataConnectorEnablment = true;
        
        $scope.updateData();
    };

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
        $scope.editing = false;
    };

    $scope.saveConnectorChanges = function( connector ){
        connector.attributes.size = $scope._selectedSize;
        connector.attributes.unit = $scope._selectedUnit.name;
        $data_connector_api.editConnector( connector );
        $scope.stopEditing();
    };

    $scope.$watch( 'name', function(){
        $scope.updateData();
    });
    
    $scope.$watch( 'data_connector', $scope.updateData );
    
    $scope.$watch( 'volumeVars.clone', function( newVal ){
        
        if ( angular.isDefined( newVal ) && newVal !== null ){
            $scope.data_connector = newVal.data_connector;
        }
    });
    
    $scope.$watch( 'volumeVars.creating', function( newVal ){
        if ( newVal === true ){
            init();
        }
    });
    
    $scope.$watch( 'volumeVars.editing', function( newVal ){
        
        if ( newVal === true ){
            $scope.name = $scope.volumeVars.selectedVolume.name;
            $scope.editing = false;
            $scope.connectors = $data_connector_api.connectors;
            $scope.data_connector = $scope.volumeVars.selectedVolume.data_connector;
            
            $scope.nameEnablement = false;
            $scope.dataConnectorEnablment = false;
            $scope.updateData();
        }
    });

}]);
