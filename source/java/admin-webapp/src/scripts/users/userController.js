angular.module( 'user-page' ).controller( 'userController', ['$scope', '$user_service', '$authorization', function( $scope, $user_service, $authorization ){

    $scope.actionLabel = 'Actions';
    $scope.actionItems = [{name: 'Edit User'},{name: 'Delete User'},{name: 'Disable User'}];
    $scope.users = [];
    $scope.creating = false;

    $scope.createNewUser = function(){
        $scope.creating = true;
    };
    
    $scope.actionSelected = function( action ){
    };

    $scope.usersReturned = function( response ){
        $scope.users = eval( response );
    };

    $scope.isAllowed = function( permission ){
        return $authorization.isAllowed( permission );
    };

    $user_service.getUsers( $scope.usersReturned );
    
    $scope.$on( 'fds::user_done_editing', function(){
        $scope.creating = false;
        $user_service.getUsers( $scope.usersReturned );
    });

}]);
