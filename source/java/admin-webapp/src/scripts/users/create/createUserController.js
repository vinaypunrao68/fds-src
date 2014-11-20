angular.module( 'user-page' ).controller( 'createUserController', ['$scope', '$tenant_service', '$user_service', function( $scope, $tenant_api, $user_api ){
    
    $scope.tenants = [];
    $scope.editing = false;

    $scope.tenantsReturned = function( tenants ){
        $scope.tenants = tenants;
        
        for ( var i = 0; $scope.editing === true && i < $scope.tenants.length && angular.isDefined( $scope.userVars.selectedUser.tenant ); i++ ){
            if ( $scope.tenants[i].name === $scope.userVars.selectedUser.tenant.name ){
                $scope.tenant = $scope.tenants[i];
            }
        }
    };

    $scope.init = function(){

        $scope.nameInit = true;
        $scope.passInit = true;
        $scope.confirmInit = true;

        $scope.tenantLabel = 'Choose Tenant';
        
        $scope.tenants = [];

        $tenant_api.getTenants( $scope.tenantsReturned );
    };
    
    $scope.setForEdit = function(){
        
        var user = $scope.userVars.selectedUser;
        $scope.name = user.identifier;
        $scope.password = '******';
        $scope.editing = true;
    };
    
    $scope.clearFields = function(){
        
        $scope.name = '';
        $scope.password = '';
        $scope.confirm = '';
        $scope.passwordError = false;
        $scope.tenant = undefined;
        
        $scope.editing = false;
    };

    $scope.cancel = function(){
        $scope.init();
        $scope.clearFields();
        
        $scope.userVars.back();
    };

    $scope.clearPasswordError = function(){
        $scope.passwordError = false;
    };

    $scope.usernameTyped = function( event ){

        if ( event.keyCode === 9 ){
            return;
        }
        $scope.nameInit = false;
    };

    $scope.passwordTyped = function( event ){

        if ( event.keyCode === 9 ){
            return;
        }

        $scope.passInit = false;
        $scope.clearPasswordError();
    };

    $scope.confirmTyped = function( event ){

        if ( event.keyCode === 9 ){
            return;
        }

        $scope.confirmInit = false;
        $scope.clearPasswordError();
    };
    
    $scope.edit = function(){
        
        var user = $scope.userVars.selectedUser;
        
        // check if an edit is necessary
        if ( $scope.userVars.selectedUser.tenant.id === $scope.tenant.id ){
            return;
        }
        
        // right now we only support the user in one tenant so we remove, then add.
        $tenant_api.revokeUser( user.tenant, user.id, function(){
            $tenant_api.attachUser( $scope.tenant, user.id, function(){
                $scope.cancel();
            });
        });
    };
    
    $scope.save = function(){
        
        if ( $scope.editing === true ){
            $scope.edit();
            return;
        }
        
        if ( $scope.password !== $scope.confirm ){
            $scope.passwordError = 'Passwords do not match';
            return;
        }

        $user_api.createUser( $scope.name, $scope.password,
            function( item ){

                if ( angular.isDefined( $scope.tenant ) ){

                        $tenant_api.attachUser( $scope.tenant, item.id, function(){
                    }, function(){} );
                }
            },
            function( response ){
                $scope.passwordError = response;
            }).then( function(){
                
                $scope.cancel();
            
            });
    };

    $scope.$watch( 'userVars.selectedUser', function( newVal ){
        
        $scope.init();
        
        if ( !angular.isDefined( newVal ) ){
            $scope.clearFields();
        }
        else {
            $scope.setForEdit();
        }
    });

}]);
