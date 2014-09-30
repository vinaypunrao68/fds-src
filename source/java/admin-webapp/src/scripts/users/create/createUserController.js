angular.module( 'user-page' ).controller( 'createUserController', ['$scope', '$tenant_service', '$user_service', function( $scope, $tenant_api, $user_api ){
    

    $scope.init = function(){
        $scope.name = '';
        $scope.password = '';
        $scope.confirm = '';
        $scope.passwordError = false;

        $scope.nameInit = true;
        $scope.passInit = true;
        $scope.confirmInit = true;
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

    $scope.usernameTyped = function(){
        $scope.nameInit = false;
    };

    $scope.passwordTyped = function(){
        $scope.passInit = false;
        $scope.clearPasswordError();
    };

    $scope.confirmTyped = function(){
        $scope.confirmInit = false;
        $scope.clearPasswordError();
    };
    
    $scope.save = function(){
        
        if ( $scope.password !== $scope.confirm ){
            $scope.passwordError = 'Passwords do not match';
            return;
        }

        $user_api.createUser( $scope.name, $scope.password,
            function(){
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
