angular.module( 'qos' ).directive( 'qosPanel', function(){
    
    return {
        restrict: 'E',
        replace: true,
        transclude: false,
        templateUrl: 'scripts/directives/fds-components/qos/qos.html',
        scope: { qos: '=ngModel', saveOnly: '@' },
        controller: function( $scope, $filter, $volume_api ){
            
            $scope.custom = { priority: 5, limit: 0, sla: 0, name: $filter( 'translate' )( 'common.l_custom' ) };
            
            $scope.qos = {};
            $scope.editing = false;           
            
            $scope.limitChoices = [100,200,300,400,500,750,1000,2000,3000,0];

            var rationalizeWithPresets = function(){
                
                if ( !angular.isDefined( $scope.qos ) || !angular.isDefined( $scope.qos.sla ) ){
                    $scope.qos = $scope.presets[1];
                    return;
                }
                
                $scope.qosPreset = undefined;
                
                for ( var i = 0; angular.isDefined( $scope.presets ) && i < $scope.presets.length-1; i++ ) {
                    
                    var preset = $scope.presets[i];
                    
                    if ( $scope.qos.priority === preset.priority &&
                        $scope.qos.sla === preset.sla &&
                        $scope.qos.limit === preset.limit ){

                        $scope.qosPreset = preset;
                    }
                }
                
                if ( $scope.qosPreset === undefined ){
                    $scope.qosPreset = $scope.presets[ $scope.presets.length - 1 ];
                }
            };
            
            var init = function(){

                $volume_api.getQosPolicyPresets( function( presets ){
                    
                    $scope.presets = presets;
                    $scope.presets.push( $scope.custom );
                    
                    rationalizeWithPresets();
                });
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

                $scope.qos = newVal;
            });
         
            init();
        }
    };
    
});