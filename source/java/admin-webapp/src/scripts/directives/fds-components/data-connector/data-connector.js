angular.module( 'volumes' ).directive( 'connectorPanel', function(){
    
    return {
        restrict: 'E',
        replace: true,
        transclude: false,
        templateUrl: 'scripts/directives/fds-components/data-connector/data-connector.html',
        scope: { setting: '=ngModel', editable: '@', enable: '=?'},
        controller: function( $scope, $data_connector_api ){
            
            $scope.sizes = [{name: 'MB'}, {name:'GB'}, {name:'TB'},{name:'PB'}];
            $scope.connectors = $data_connector_api.connectors;
            $scope._selectedSize = 10;
            $scope._selectedUnit = $scope.sizes[1];

            var findUnit = function(){

                if ( !angular.isDefined( $scope.setting.capacity ) ){
                    return;
                }
                
                for ( var i = 0; i < $scope.sizes.length; i++ ){
                    if ( $scope.sizes[i].name == $scope.setting.capacity.unit ){
                        $scope._selectedUnit = $scope.sizes[i];
                    }
                }
            };
            
            var refreshSelection = function(){
                 
                if ( angular.isDefined( $scope.setting.capacity ) ){
                    $scope.setting.capacity.size = $scope._selectedSize;
                    $scope.setting.capacity.unit = $scope._selectedUnit.name;
                }
            };
            
            $scope.$on( 'fds::refresh', refreshSelection );
            
            $scope.$watch( 'connectors', function( newVal ){
                
                if ( !angular.isDefined( newVal ) || !angular.isDefined( newVal.type ) ){
                    $scope.setting = $scope.connectors[1];
                    return;
                }
                
                $scope.$emit( 'fds::data_connector_changed' );
                $scope.$emit( 'change' );
                
                if ( $scope.setting.capacity ){
                    $scope._selectedSize = $scope.connectors.attributes.size;
                    findUnit();
                }
                
            });
            
        }
    };
    
});