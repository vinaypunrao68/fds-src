angular.module( 'tenant' ).controller( 'tenantController', ['$scope', '$tenant_service', function( $scope, $tenant_api ){

    $scope.creating = false;
    $scope.viewing = false;
    $scope.checkall = false;
    $scope.actions = [{name: 'Delete tenants'}];
    $scope.tenants = [];
    $scope.actionLabel = 'Actions';

    $scope.createNewTenant = function(){
        $scope.tenantVars.next( 'createtenant' );
    };

    $scope.actionSelected = function( action ){


    };

    $scope.tenantCheckChanged = function(){

        var checked = 0;

        for ( var i = 0; i < $scope.tenants.length; i++ ){
            if ( $scope.tenants[i].checked === true ){
                checked++;
            }
        }

        if ( checked === $scope.tenants.length ){
            $scope.checkall = true;
        }
        else if ( checked === 0 ){
            $scope.checkall = false;
        }
        else{
            $scope.checkall = 'partial';
        }
    };

    $scope.$watch( 'checkall', function( newVal ){

        if ( newVal === 'partial' ){
            return;
        }

        for ( var i = 0; i < $scope.tenants.length; i++ ){
            $scope.tenants[i].checked = newVal;
        }
    });
    
    $scope.$watch( 'tenantVars.index', function( newVal ){
        
        if ( newVal === 0 ){
            $scope.refresh();
        }
    });

    $scope.refresh = function(){
        $tenant_api.getTenants( function( tenants ){ $scope.tenants = tenants;} );
    };

    $scope.refresh();
}]);
