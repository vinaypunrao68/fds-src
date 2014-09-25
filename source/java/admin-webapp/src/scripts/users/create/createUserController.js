angular.module( 'user-page' ).controller( 'createUserController', ['$scope', '$tenant_service', '$user_service', function( $scope, $tenant_api, $user_api ){
    
    $scope.name = '';
    $scope.name = '';
    $scope.password = '';
    $scope.confirm = '';

    $scope.cancel = function(){
        $scope.name = '';
        $scope.password = '';
        $scope.confirm = '';
        
        $scope.$emit( 'fds::user_done_editing' );
    };    
    
    $scope.save = function(){
        
        $scope.cancel();
    };

}]);