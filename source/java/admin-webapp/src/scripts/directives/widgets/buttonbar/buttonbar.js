angular.module( 'form-directives' ).directive( 'buttonBar', function(){
    
    return {
        restrict: 'E',
        replace: true,
        transclude: false,
        templateUrl: 'scripts/directives/widgets/buttonbar/buttonbar.html',
        // buttons [{label: <>, value: <>}...]
        scope: { buttons: '=', selected: '=ngModel', disabled: '=?' },
        controller: function( $scope ){
            
            if ( !angular.isDefined( $scope.disabled ) ){
                $scope.disabled = false;
            }
            
            $scope.selectButton = function( button ){
                
                if ( $scope.disabled === true ){
                    return;
                }
                
                $scope.selected = button;
            };
        }
    };
});