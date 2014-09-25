angular.module( 'main' ).controller( 'accountController', ['$scope', '$authentication', '$authorization', '$user_service', function( $scope, $authentication, $authorization, $user_service ){

    $scope.emailAddress = 'admin@example.com';
    $scope.tempEmailAddress = $scope.emailAddress;
    $scope.changePasswordError = false;
    $scope.username = $authorization.getUsername();

    $scope.changePassword = function(){

        $scope.changePasswordError = false;

        if ( $scope.newPassword !== $scope.confirmPassword ){
            $scope.changePasswordError = 'Passwords do not match.';
            return;
        }

        $user_service.changePassword( $authorization.user.userId, $scope.newPassword,
            function(){
                $scope.newPassword = '';
                $scope.confirmPassword = '';
                $scope.password_changing = false;
            },
            function( error, code ){
                $scope.changePasswordError = code + ' - ' + error.message;
            });

    };

}]);
