angular.module( 'volumes' ).directive( 'connectorPanel', function(){
    
    return {
        restrict: 'E',
        replace: true,
        transclude: false,
        templateUrl: 'scripts/directives/fds-components/data-connector/data-connector.html',
        scope: { dataConnector: '=ngModel', editable: '@', enable: '=?'},
        controller: function( $scope, $data_connector_api ){
            
            $scope.sizes = [{name:'GB'}, {name:'TB'},{name:'PB'}];
            $scope.connectors = $data_connector_api.connectors;
            $scope._selectedSize = 1;
            $scope._selectedUnit = $scope.sizes[0];

            var findUnit = function(){

                if ( !angular.isDefined( $scope.dataConnector.attributes ) ){
                    return;
                }
                
                for ( var i = 0; i < $scope.sizes.length; i++ ){
                    if ( $scope.sizes[i].name == $scope.dataConnector.attributes.size ){
                        $scope._selectedUnit = $scope.sizes[i];
                    }
                }
            };
            
            var refreshSelection = function(){
                 
                if ( angular.isDefined( $scope.dataConnector.attributes ) ){
                    $scope.dataConnector.attributes.size = $scope._selectedSize;
                    $scope.dataConnector.attributes.unit = $scope._selectedUnit.name;
                }
            };
            
            $scope.$on( 'fds::refresh', refreshSelection );
            
            $scope.$watch( 'dataConnector', function( newVal ){
                
                if ( !angular.isDefined( newVal ) || !angular.isDefined( newVal.type ) ){
                    $scope.dataConnector = $scope.connectors[1];
                    return;
                }
                
                $scope.$emit( 'fds::data_connector_changed' );
                $scope.$emit( 'change' );
                
                if ( $scope.dataConnector.attributes ){
                    $scope._selectedSize = $scope.dataConnector.attributes.size;
                    findUnit();
                }
                
            });
            
        }
    };
    
});