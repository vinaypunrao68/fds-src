angular.module( 'form-directives' ).directive( 'buttonBar', function(){
    
    return {
        restrict: 'E',
        replace: true,
        transclude: false,
        templateUrl: 'scripts/directives/widgets/buttonbar/buttonbar.html',
        // buttons [{label: <>, value: <>}...]
        scope: { buttons: '=', selected: '=ngModel' },
        controller: function( $scope ){
            
            $scope.selectButton = function( button ){
                $scope.selected = button;
            };
        }
    };
});