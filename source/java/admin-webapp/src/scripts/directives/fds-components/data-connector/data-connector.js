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
            
            $scope.get_option = function( value ){
                return { 'name': value };
            };
            
            $scope.refreshSelection = function(){
                 
                if ( angular.isDefined( $scope.volumeType.capacity ) ){
                    $scope.volumeType.capacity.value = $scope._selectedSize;
                    $scope.volumeType.capacity.unit = $scope._selectedUnit.name;
                }
                
                // this means its iSCSI
                if ( angular.isDefined( $scope.volumeType.target ) ){
                    $scope.volumeType.target.incomingUsers = [{username: $scope._username, password: $scope._password}];
                }
                
                // this is the NFS ip filters
                if ( angular.isDefined( $scope.volumeType.filters )){
                    
                    $scope.volumeType.filters = [];
                    
                    for ( var fI = 0; fI < $scope._ip_filters; fI++ ){
                        $scope.volumeType.filters.push( { 'pattern': { 'value': $scope._ip_filters[fI] }, 'mode': 'ALLOW' } );
                    }
                }
                
                // this is the NFS options
                if ( angular.isDefined( $scope.volumeType.options ) ){
                    
                    $scope.volumeType.options = [];
                    
                    if ( $scope._acls === false ){
                        $scope.volumeType.options = $scope.get_option( 'no_acls' );
                    }
                    else {
                        $scope.volumeType.options = $scope.get_option( 'acls' );
                    }
                    
                    if ( $scope._async === false ){
                        $scope.volumeType.options = $scope.get_option( 'sync' );
                    }
                    else {
                        $scope.volumeType.options = $scope.get_option( 'async' );
                    }
                    
                    if ( $scope._root_squash === false ){
                        $scope.volumeType.options = $scope.get_option( 'squash' );
                    }
                    else {
                        $scope.volumeType.options = $scope.get_option( 'squash_all' );
                    }
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
                
                // this means its iscsi
                if ( angular.isDefined( $scope.volumeType.target ) && 
                     angular.isDefined( $scope.volumeType.target.incomingUsers ) && 
                     $scope.volumeType.target.incomingUsers.length > 0 ){
                    
                    $scope._username = $scope.volumeType.target.incomingUsers[0].username;
                    $scope._password = $scope.volumeType.target.incomingUsers[0].password;
                }
                
                // this means it's NFS
                if ( angular.isDefned( $scope.volumeType.filters ) && 
                    $scope.volumeType.filters.length > 0 ){
                    
                    for ( var i = 0; i < $scope.volumeType.filters.length; i++ ){
                        var filter = $scope.volumeType.filters[i];
                        
                        // the UI does not show the DENY portions yet.
                        if ( filter.mode !== 'ALLOW' ){
                            continue;
                        }
                        
                        $scope._ip_filters.push( filter.pattern.value );
                    }
                }
                
                // this is the NFS options
                if ( angular.isDefined( $scope.volumeType.options ) ){
                    
                    for ( var oI = 0; oI < $scope.volumeType.options.length; oI++ ){
                        
                        var opt = $scope.volumeType.options[oI];
                        
                        if ( opt.name === 'sync' ){
                            $scope._async = false;
                        }
                        else if ( opt.name === 'async' ){
                            $scope._async = true;
                        }
                        else if ( opt.name === 'squash' ){
                            $scope._root_squash = true;
                        }
                        else if ( opt.name === 'squash_all' ){
                            $scope._root_squash = false;
                        }
                        else if ( opt.name === 'acl' ){
                            $scope._acls = true;
                        }
                        else if ( opt.name === 'no_acl' ){
                            $scope._acls = false;
                        }
                    }
                }
                
            });
            
            var init = function(){
                
                $data_connector_api.getVolumeTypes( typesReturned );
            };
            
            init();
        }
    };
    
});