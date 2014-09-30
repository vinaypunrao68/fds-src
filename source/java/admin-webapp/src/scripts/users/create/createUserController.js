angular.module( 'user-page' ).controller( 'createUserController', ['$scope', '$tenant_service', '$user_service', function( $scope, $tenant_api, $user_api ){
    
    $scope.tenants = [];

    $scope.tenantsReturned = function( tenants ){
        $scope.tenants = tenants;
    };

    $scope.init = function(){
        $scope.name = '';
        $scope.password = '';
        $scope.confirm = '';
        $scope.passwordError = false;
        $scope.tenant = undefined;
        $scope.tenantLabel = 'Choose Tenant';

        $scope.nameInit = true;
        $scope.passInit = true;
        $scope.confirmInit = true;

        $scope.tenants = [];

        $tenant_api.getTenants( $scope.tenantsReturned );
    };

    $scope.cancel = function(){
        $scope.name = '';
        $scope.password = '';
        $scope.confirm = '';
        
        $scope.$emit( 'fds::user_done_editing' );
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
    
    $scope.save = function(){
        
        if ( $scope.password !== $scope.confirm ){
            $scope.passwordError = 'Passwords do not match';
            return;
        }

        $user_api.createUser( $scope.name, $scope.password,
            function( item ){

                if ( angular.isDefined( $scope.tenant ) ){

                    $tenant_api.attachUser( $scope.tenant, item.id, function(){}, function(){} );
                }

                $scope.cancel();
            },
            function( response ){
                $scope.passwordError = response;
            });
    };

    $scope.$on( 'fds::page_shown', function(){
        $scope.init();
    });

    $scope.$on( 'fds::page_hidden', function(){
    });

    $scope.init();
}]);
