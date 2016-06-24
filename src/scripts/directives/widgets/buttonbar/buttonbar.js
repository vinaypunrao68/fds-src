angular.module( 'form-directives' ).directive( 'buttonBar', function(){
    
    return {
        restrict: 'E',
        replace: true,
        transclude: false,
        templateUrl: 'scripts/directives/widgets/buttonbar/buttonbar.html',
        // buttons [{label: <>, value: <>}...]
        scope: { buttons: '=', selected: '=ngModel', disabled: '=?', label: '@' },
        controller: function( $scope ){
            
            if ( !angular.isDefined( $scope.disabled ) ){
                $scope.disabled = false;
            }
            
            if ( !angular.isDefined( $scope.label ) ){
                $scope.label = 'label';
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