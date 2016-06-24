angular.module( 'form-directives' ).directive( 'triStateCheck', function(){

    return {
        restrict: 'E',
        replace: true,
        transclude: false,
        templateUrl: 'scripts/directives/widgets/tristatecheck/tristatecheck.html',
        scope: { checkState: '=', enabled: '=?'},
        controller: function( $scope ){

            if ( !angular.isDefined( $scope.enabled ) ){
                $scope.enabled = true;
            }
            
            var UNCHECKED = false;
            var CHECKED = true;
            var PARTIAL = 'partial';

            var notify = function(){
                $scope.$emit( 'change' );
            };

            $scope.uncheck = function(){
                
                if ( $scope.enabled === false ){
                    return;
                }
                
                $scope.checkState = UNCHECKED;
                notify();
            };

            $scope.check = function(){
                
                if ( $scope.enabled === false ){
                    return;
                }
                
                $scope.checkState = CHECKED;
                notify();
            };

            $scope.partial = function(){
                $scope.checkState = PARTIAL;
            };
        }
    };

});
