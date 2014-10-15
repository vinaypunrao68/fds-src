angular.module( 'form-directives' ).directive( 'fdsRadio', function(){
    
    return {
        restrict: 'E',
        replace: true,
        transclude: false,
        templateUrl: 'scripts/directives/widgets/fds-radio/fds-radio.html',
        scope: { selectedValue: '=ngModel', name: '@', value: '@', label: '@' },
        controller: function( $scope ){
        }
    };
});