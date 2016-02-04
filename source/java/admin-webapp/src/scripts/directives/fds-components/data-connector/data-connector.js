angular.module( 'volumes' ).directive( 'connectorPanel', function(){
    
    return {
        restrict: 'E',
        replace: true,
        transclude: false,
        templateUrl: 'scripts/directives/fds-components/data-connector/data-connector.html',
        scope: { volumeType: '=ngModel', editable: '@', enable: '=?'},
        controller: function( $scope, $data_connector_api, $byte_converter ){
            
            var SYNC = 'sync';
            var ASYNC = 'async';
            var ACLS = 'acl';
            var NO_ACLS = 'noacl';
            var ROOT_SQUASH = 'root_squash';
            var NO_ROOT_SQUASH = 'no_root_squash';
            
            $scope.sizes = [{name: 'MB'}, {name:'GB'}, {name:'TB'},{name:'PB'}];
            $scope.types = [];
            $scope._selectedSize = 10;
            $scope._selectedUnit = $scope.sizes[1];
            $scope._acls = false;
            $scope._root_squash = false;
            $scope._async = true;
            $scope._clients = '*';
            $scope.chapRequired = false;
            $scope.ipFilterRequired = true;

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
                    $scope.volumeType.target.incomingUsers = [];
                    
                    if ( angular.isDefined( $scope._username ) && $scope._username.trim() !== "" ){
                        $scope.volumeType.target.incomingUsers = [{username: $scope._username, password: $scope._password}];
                    }
                }
                
                // this is the NFS ip filters
                if ( angular.isDefined( $scope.volumeType.clients )){
                    $scope.volumeType.clients = $scope._clients;
                }
                
                // this is the NFS options
                if ( angular.isDefined( $scope.volumeType.options ) ){
                    
                    var optionList = [];
                    
                    if ( $scope._acls === true ){
                        optionList.push( ACLS );
                    }
                    else {
                        optionList.push( NO_ACLS );
                    }
                    
                    if ( $scope._async === false ){
                        optionList.push( SYNC );
                    }
                    else {
                        optionList.push( ASYNC );
                    }
                    
                    if ( $scope._root_squash === false ){
                        optionList.push( NO_ROOT_SQUASH );
                    }
                    else {
                        optionList.push( ROOT_SQUASH );
                    }
                    
                    var str = '';
                    
                    for ( var i = 0; i < optionList.length; i++ ){
                        
                        str += optionList[i];
                        
                        if ( (i+1) != optionList.length ){
                            str += ',';
                        }
                    }
                    
                    $scope.volumeType.options = str;
                }
            };
            
            var typesReturned = function( types ){
                
                $scope.types = types;
                
                if ( $scope.types.length > 0 ){
                    $scope.volumeType = $scope.types[0];
                }
            };
            
            $scope.$on( 'fds::refresh', $scope.refreshSelection );
            
            $scope.$watch( 'volumeType.clients', function( newVal ){
                
                if ( !angular.isDefined( newVal ) || newVal.trim().length === 0 ){
                    return;
                }
                
                $scope._clients = newVal;
            });
            
            $scope.$watch( 'volumeType', function( newVal ){
            
                if ( !angular.isDefined( newVal ) || !angular.isDefined( newVal.type ) ){
                    $scope.volumeType = $scope.types[1];
                    return;
                }
                
                $scope.$emit( 'fds::data_connector_changed' );
                $scope.$emit( 'change' );
                
                if ( angular.isDefined( $scope.volumeType.capacity ) ){
                    
                    var b_str = $byte_converter.convertFromUnitsToString( $scope.volumeType.capacity.value, $scope.volumeType.capacity.units );
                    var b_num = parseInt( b_str.split( ' ' )[0] );
                    
                    $scope._selectedSize = b_num;
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
                if ( angular.isDefined( $scope.volumeType.clients ) && $scope.volumeType.clients.trim() !== '' ){

                    $scope._clients = $scope.volumeType.clients;
                }
                
                // this is the NFS options
                if ( angular.isDefined( $scope.volumeType.options ) ){
                    
                    var optionList = $scope.volumeType.options.split( ',' );

                    for ( var oI = 0; oI < optionList.length; oI++ ){
                        
                        var opt = optionList[oI];
                        
                        if ( opt === SYNC ){
                            $scope._async = false;
                        }
                        else if ( opt === ASYNC ){
                            $scope._async = true;
                        }
                        else if ( opt === ROOT_SQUASH ){
                            $scope._root_squash = true;
                        }
                        else if ( opt === NO_ROOT_SQUASH ){
                            $scope._root_squash = false;
                        }
                        else if ( opt === ACLS ){
                            $scope._acls = true;
                        }
                        else if ( opt === NO_ACLS ){
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