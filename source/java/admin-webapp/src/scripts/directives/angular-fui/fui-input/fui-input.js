angular.module( 'angular-fui' ).directive( 'fuiInput', function(){

    return {
        restrict: 'E',
        replace: true,
        transclude: false,
        templateUrl: 'scripts/directives/angular-fui/fui-input/fui-input.html',
        scope: { aliasedModel : '=ngModel', placeholder: '@', iconClass: '@' },
        controller: function( $scope ){

        }
    };
});
