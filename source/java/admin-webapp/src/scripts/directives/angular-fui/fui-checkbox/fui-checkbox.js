angular.module( 'angular-fui' ).directive( 'fuiCheckbox', function(){

    return {
        restrict: 'E',
        replace: true,
        transclude: false,
        scope: { label: '@', checked: '=' },
        templateUrl: 'scripts/directives/angular-fui/fui-checkbox/fui-checkbox.html',
        controller: function( $scope ){

            if ( !angular.isDefined( $scope.checked ) ){
                $scope.checked = false;
            }
        }
    };
});
