angular.module( 'tenant' ).controller( 'createTenantController', ['$scope', '$tenant_service', function( $scope, $tenant_api ){

    var done = function(){
        $scope.name = '';
        $scope.tenantVars.back();
    };

    $scope.cancel = function(){
        done();
    };

    $scope.save = function(){

        var tenant = { 
            id: {
                name: $scope.name
            },
        };

        $tenant_api.createTenant( tenant, done );
    };

}]);
