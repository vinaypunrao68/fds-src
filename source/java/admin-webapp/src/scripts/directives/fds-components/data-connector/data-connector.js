angular.module( 'volumes' ).directive( 'connectorPanel', function(){
    
    return {
        restrict: 'E',
        replace: true,
        transclude: false,
        templateUrl: 'scripts/directives/fds-components/data-connector/data-connector.html',
        scope: { volumeType: '=ngModel', editable: '@', enable: '=?'},
        controller: function( $scope, $data_connector_api ){
            
            $scope.sizes = [{name: 'MB'}, {name:'GB'}, {name:'TB'},{name:'PB'}];
            $scope.types = [];
            $scope._selectedSize = 10;
            $scope._selectedUnit = $scope.sizes[1];
            $scope._acls = false;
            $scope._root_squash = false;
            $scope._async = false;
            $scope._ip_filters = [];

            var findUnit = function(){

                if ( !angular.isDefined( $scope.volumeType.capacity ) ){
                    return;
                }
                
                for ( var i = 0; i < $scope.sizes.length; i++ ){
                    if ( $scope.sizes[i].name == $scope.volumeType.capacity.unit ){
                        $scope._selectedUnit = $scope.sizes[i];
                    }
                }
            };
            
            $scope.refreshSelection = function(){
                 
                if ( angular.isDefined( $scope.volumeType.capacity ) ){
                    $scope.volumeType.capacity.value = $scope._selectedSize;
                    $scope.volumeType.capacity.unit = $scope._selectedUnit.name;
                }
                
                if ( angular.isDefined( $scope.volumeType.target ) ){
                    $scope.volumeType.target.incomingUsers = [{username: $scope._username, password: $scope._password}];
                }
            };
            
            var typesReturned = function( types ){
                
                $scope.types = types;
                
                if ( $scope.types.length > 0 ){
                    $scope.volumeType = $scope.types[0];
                }
            };
            
            $scope.$on( 'fds::refresh', $scope.refreshSelection );
            
            $scope.$watch( 'volumeType', function( newVal ){
                
                if ( !angular.isDefined( newVal ) || !angular.isDefined( newVal.type ) ){
                    $scope.volumeType = $scope.types[1];
                    return;
                }
                
                $scope.$emit( 'fds::data_connector_changed' );
                $scope.$emit( 'change' );
                
                if ( angular.isDefined( $scope.volumeType.capacity ) ){
                    $scope._selectedSize = $scope.volumeType.capacity.value;
                    findUnit();
                }
                
                if ( angular.isDefined( $scope.volumeType.target ) && 
                     angular.isDefined( $scope.volumeType.target.incomingUsers ) && 
                     $scope.volumeType.target.incomingUsers.length > 0 ){
                    
                    $scope._username = $scope.volumeType.target.incomingUsers[0].username;
                    $scope._password = $scope.volumeType.target.incomingUsers[0].password;
                }
                
            });
            
            var init = function(){
                
                $data_connector_api.getVolumeTypes( typesReturned );
            };
            
            init();
        }
    };
    
});