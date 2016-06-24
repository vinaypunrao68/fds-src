angular.module( 'admin-settings' ).controller( 'profileController', ['$scope','$authentication', function( $scope, $authentication ){

    $scope.editing = false;

    $scope.user = $authentication.getUserInfo();

    $scope.changePassword = function(){
        $scope.changepassword = false;
    };

    $scope.edit = function(){
        $scope.editing = true;
        $scope._username = $scope.user.name;
        $scope._email = $scope.user.email;

    };

    $scope.cancel = function(){
        $scope.editing = false;
    };


    $scope.save = function(){
        $scope.user.name = $scope._username;
        $scope.user.email = $scope._email;
        $scope.cancel();
    };

}]);
