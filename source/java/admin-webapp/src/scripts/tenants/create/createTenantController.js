angular.module( 'tenant' ).controller( 'createTenantController', ['$scope', '$tenant_service', function( $scope, $tenant_api ){

    var done = function(){
        $scope.$emit( 'fds::tenant_done_editing' );
    };

    $scope.cancel = function(){
        done();
    };

    $scope.save = function(){

        var tenant = {name: $scope.name};

        $tenant_api.createTenant( tenant, done );
    };

}]);
