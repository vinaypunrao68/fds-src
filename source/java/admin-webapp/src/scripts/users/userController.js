angular.module( 'user-page' ).controller( 'userController', ['$scope', '$user_service', '$authorization', function( $scope, $user_service, $authorization ){

    $scope.actionLabel = 'Actions';
    $scope.actionItems = [{ name: 'Actions' },{name: 'Edit User'},{name: 'Delete User'},{name: 'Disable User'}];
    $scope.users = [];

    $scope.actionSelected = function( action ){
    };

    $scope.usersReturned = function( response ){
        $scope.users = eval( response );
    };

    $scope.isAllowed = function( permission ){
        return $authorization.isAllowed( permission );
    };

    $user_service.getUsers( $scope.usersReturned );

}]);
