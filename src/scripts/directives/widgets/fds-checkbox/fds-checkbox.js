angular.module( 'form-directives' ).directive( 'fdsCheckbox', function(){
    
    return {
        restrict: 'E',
        replace: true,
        transclude: false,
        templateUrl: 'scripts/directives/widgets/fds-checkbox/fds-checkbox.html',
        scope: { selectedValue: '=ngModel', label: '@', enabled: '=?', iconClass: '@' },
        controller: function( $scope ){
            
            var localEnabled = true;
            
            if ( !angular.isDefined( $scope.enabled ) ){
                $scope.enabled = localEnabled;    
            }
            
            $scope.buttonClicked = function(){
                
                if ( $scope.enabled === true ){
                    
                    $scope.selectedValue = !$scope.selectedValue;
                }  
            };
        },
        link: function( $scope, $element, $attrs ){
        }
    };
});