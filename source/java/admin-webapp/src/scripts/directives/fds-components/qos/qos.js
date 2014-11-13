angular.module( 'qos' ).directive( 'qosPanel', function(){
    
    return {
        restrict: 'E',
        replace: true,
        transclude: false,
        templateUrl: 'scripts/directives/fds-components/qos/qos.html',
        scope: { qos: '=ngModel' },
        controller: function( $scope ){

            $scope.editing = false;
            
            if ( !angular.isDefined( $scope.qos.capacity ) ){
                $scope.qos.capacity = 0;
            }
            
            if ( !angular.isDefined( $scope.qos.priority ) ){
                $scope.qos.priority = 10;
            }
            
            if ( !angular.isDefined( $scope.qos.limit ) ){
                $scope.qos.limit = 0;
            }            
            
            $scope.limitChoices = [100,200,300,400,500,750,1000,2000,3000,0];

            $scope.limitSliderLabel = function( value ){

                if ( value === 0 ){
                    return '\u221E';
                }

                return value;
            };

            $scope.doneEditing = function(){
                $scope.$emit( 'fds::qos_changed' );
                $scope.$emit( 'change' );
                $scope.editing = false;
            };

            $scope.$on( 'fds::page_shown', function(){
                $scope.$broadcast( 'fds::fui-slider-refresh' );
            });
            
        }
    };
    
});