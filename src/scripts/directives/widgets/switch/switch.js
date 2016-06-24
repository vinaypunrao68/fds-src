angular.module( 'form-directives' ).directive( 'switch', function(){
    
    return {
        restrict: 'E',
        replace: true,
        transclude: false,
        templateUrl: 'scripts/directives/widgets/switch/switch.html',
        scope: { active: '=ngModel', onText: '@', offText: '@', disabled: '=?' },
        controller: function( $scope ){
            
            if ( !angular.isDefined( $scope.disabled ) ){
                $scope.disabled = false;
            }
            
            if ( !angular.isDefined( $scope.offText ) ){
                $scope.offText = 'OFF';
            }
            
            if ( !angular.isDefined( $scope.onText ) ){
                $scope.onText = 'ON';
            }
            
            var offWidth = measureText( $scope.offText, 13 ).width;
            var onWidth = measureText( $scope.onText, 13 ).width;
            
            $scope.calcWidth = offWidth + 30;
            
            if ( onWidth > offWidth ){
                $scope.calcWidth = onWidth + 30;
            }
            
            $scope.switch = function(){
                
                if ( $scope.disabled === true ){
                    return;
                }
                
                $scope.active = !$scope.active;
            };
        }
    };
    
});