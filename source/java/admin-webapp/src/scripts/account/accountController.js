angular.module( 'main' ).controller( 'accountController', ['$scope', '$authentication', '$authorization', '$user_service', function( $scope, $authentication, $authorization, $user_service ){

    $scope.emailAddress = 'admin@example.com';
    $scope.tempEmailAddress = $scope.emailAddress;
    $scope.changePasswordError = false;
    $scope.username = $authorization.getUsername();
    $scope.password_changing = false;

    $scope.changePasswordClicked = function(){
        $scope.password_changing = true;
    };
    
    $scope.stopChangingPassword = function(){
        $scope.password_changing = false;
    };
    
    $scope.changePassword = function(){

        $scope.changePasswordError = false;

        if ( $scope.newPassword !== $scope.confirmPassword ){
            $scope.changePasswordError = 'Passwords do not match.';
            return;
        }

        var newPasswordUser = $authorization.user;
        newPasswordUser.password = $scope.newPassword;
        
        $user_service.changePassword( newPasswordUser,
            function(){
                $scope.newPassword = '';
                $scope.confirmPassword = '';
                $scope.stopChangingPassword();
            },
            function( error, code ){
                $scope.changePasswordError = code + ' - ' + error.message;
            });

    };

}]);
