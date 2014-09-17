angular.module( 'form-directives' ).directive( 'triStateCheck', function(){

    return {
        restrict: 'E',
        replace: true,
        transclude: false,
        templateUrl: 'scripts/directives/widgets/tristatecheck/tristatecheck.html',
        scope: { checkState: '='},
        controller: function( $scope ){

            var UNCHECKED = false;
            var CHECKED = true;
            var PARTIAL = 'partial';

            $scope.uncheck = function(){
                $scope.checkState = UNCHECKED;
            };

            $scope.check = function(){
                $scope.checkState = CHECKED;
            };

            $scope.partial = function(){
                $scope.checkState = PARTIAL;
            };
        }
    };

});
