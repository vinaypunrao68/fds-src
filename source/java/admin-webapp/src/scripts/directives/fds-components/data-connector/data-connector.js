angular.module( 'volumes' ).directive( 'connectorPanel', function(){
    
    return {
        restrict: 'E',
        replace: true,
        transclude: false,
        templateUrl: 'scripts/directives/fds-components/data-connector/data-connector.html',
        scope: { dataConnector: '=ngModel', editable: '@' },
        controller: function( $scope, $data_connector_api ){
            
            $scope.sizes = [{name:'GB'}, {name:'TB'},{name:'PB'}];
            $scope._selectedSize = 10;
            $scope._selectedUnit = $scope.sizes[0];
            $scope.connectors = $data_connector_api.connectors;
            
            if ( !angular.isDefined( $scope.dataConnector.type ) ){
                $scope.dataConnector = $scope.connectors[1];
            }

            $scope.editing = false;
            
            if ( !angular.isDefined( $scope.editable ) ){
                $scope.editable = 'true';
            }

            var findUnit = function( str ){

                for ( var i = 0; i < $scope.sizes.length; i++ ){
                    if ( $scope.sizes[i].name == str ){
                        return $scope.sizes[i];
                    }
                }
            };

            $scope.startEditing = function(){
                              
                if ( angular.isDefined( $scope.editingConnector ) && 
                    angular.isDefined( $scope.editingConnector.attributes ) ){
                    
                    $scope._selectedUnit = findUnit( $scope.editingConnector.attributes.unit );
                    $scope._selectedSize = parseInt( $scope.editingConnector.attributes.size );
                }
                
                for( var i = 0; i < $scope.connectors.length; i++ ){
                    if ( $scope.connectors[i].type === $scope.dataConnector.type.toUpperCase() ){
                        $scope.editingConnector = $scope.connectors[i];
                        break;
                    }
                }
                
                $scope.editing = true;
                
            };

            $scope.stopEditing = function(){
                $scope.editing = false;
            };

            $scope.saveConnectorChanges = function(){

                $scope.dataConnector = $scope.editingConnector;
                
                if ( $scope.dataConnector.type.toLowerCase() === 'block' ){
                    $scope.dataConnector.attributes = {
                        size: $scope._selectedSize,
                        unit: $scope._selectedUnit.name
                    };
                }
                
                $data_connector_api.editConnector( $scope.editingConnector );
                $scope.stopEditing();
            };

            $scope.$watch( 'dataConnector', function( newVal ){
                
                if ( !angular.isDefined( newVal ) || !angular.isDefined( newVal.type ) ){
                    return;
                }
                
                $scope.$emit( 'fds::data_connector_changed' );
                $scope.$emit( 'change' );
                
            });
            
        }
    };
    
});