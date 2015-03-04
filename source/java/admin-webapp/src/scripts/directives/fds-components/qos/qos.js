angular.module( 'qos' ).directive( 'qosPanel', function(){
    
    return {
        restrict: 'E',
        replace: true,
        transclude: false,
        templateUrl: 'scripts/directives/fds-components/qos/qos.html',
        scope: { qos: '=ngModel', saveOnly: '@' },
        controller: function( $scope, $filter ){
            
            $scope.custom = { priority: 5, limit: 0, sla: 0 };
            
            $scope.presets = [
                {
                    label: $filter( 'translate' )( 'volumes.qos.l_least_important' ),
                    value: { priority: 10, limit: 0, sla: 0 }
                },
                {
                    label: $filter( 'translate' )( 'volumes.qos.l_standard' ),
                    value: { priority: 7, limit: 0, sla: 0 }                    
                },
                {
                    label: $filter( 'translate' )( 'volumes.qos.l_most_important' ),
                    value: { priority: 1, limit: 0, sla: 0 }                    
                },
                {
                    label: $filter( 'translate' )( 'common.l_custom' ),
                    value: $scope.custom                    
                }
            ];
            
            $scope.qosPreset = $scope.presets[0];
            $scope.qos = {};
            $scope.editing = false;           
            
            $scope.limitChoices = [100,200,300,400,500,750,1000,2000,3000,0];

            var init = function(){

                if ( !angular.isDefined( $scope.qos ) || !angular.isDefined( $scope.qos.sla ) ){
                    $scope.qos = $scope.presets[1].value;
                }
                
                if ( $scope.qos.priority === $scope.presets[0].value.priority &&
                    $scope.qos.sla === $scope.presets[0].value.sla &&
                    $scope.qos.limit === $scope.presets[0].value.limit ){
                    
                    $scope.qosPreset = $scope.presets[0];
                }
                else if ( $scope.qos.priority === $scope.presets[1].value.priority &&
                    $scope.qos.sla === $scope.presets[1].value.sla &&
                    $scope.qos.limit === $scope.presets[1].value.limit ){
                    
                    $scope.qosPreset = $scope.presets[1];
                }
                else if ( $scope.qos.priority === $scope.presets[2].value.priority &&
                    $scope.qos.sla === $scope.presets[2].value.sla &&
                    $scope.qos.limit === $scope.presets[2].value.limit ){
                    
                    $scope.qosPreset = $scope.presets[2];
                }                
                else {
                    $scope.qosPreset = $scope.presets[3];
                }
            };
            
            $scope.limitSliderLabel = function( value ){

                if ( value === 0 ){
                    return '\u221E';
                }

                return value;
            };

            $scope.$on( 'fds::page_shown', function(){
                
                $scope.$broadcast( 'fds::fui-slider-refresh' );
            });
            
            $scope.$on( 'fds::qos-reinit', init );
            
            $scope.$watch( 'qosPreset', function( newVal, oldVal ){
                
                if ( newVal === oldVal || !angular.isDefined( newVal ) ){
                    return;
                }

                $scope.qos = newVal.value;
            });
         
            init();
        }
    };
    
});