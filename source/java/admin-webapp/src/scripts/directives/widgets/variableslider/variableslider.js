angular.module( 'form-directives' ).directive( 'variableSlider', function(){

    return {
        restrict: 'E',
        replace: true,
        transclude: false,
        templateUrl: 'scripts/directives/widgets/variableslider/variableslider.html',
        // data should be in format [val,val,val ... ]
        scope: { selection: '=', selections: '=', description: '@'},
        controller: function( $scope ){

            if ( !angular.isDefined( $scope.selection ) ){
                $scope.selection = $scope.selections[0];
            }

            $scope.setSelection = function( selection ){
                $scope.selection = selection;
            };
        }
    };
});
