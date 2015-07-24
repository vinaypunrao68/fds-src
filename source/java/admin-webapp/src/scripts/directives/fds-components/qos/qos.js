angular.module( 'qos' ).directive( 'qosPanel', function(){
    
    return {
        restrict: 'E',
        replace: true,
        transclude: false,
        templateUrl: 'scripts/directives/fds-components/qos/qos.html',
        scope: { qos: '=ngModel', saveOnly: '@' },
        controller: function( $scope, $filter, $volume_api ){
            
            $scope.custom = { priority: 5, iopsMin: 0, iopsMax: 0, name: $filter( 'translate' )( 'common.l_custom' ) };
            
            $scope.qos = {};
            $scope.editing = false;           
            
            $scope.limitChoices = [100,200,300,500,1000,2000,3000,5000,7500, 10000,0];
            $scope.guaranteeChoices = [0,100,200,300,500,1000,2000,3000,5000, 7500, 10000];

            var refreshSliders = function(){
                $scope.$broadcast( 'fds::fui-slider-refresh' );
            };
            
            var rationalizeWithPresets = function(){
                
                if ( !angular.isDefined( $scope.qos ) || !angular.isDefined( $scope.qos.iopsMin ) ){
                    $scope.qos = $scope.presets[1];
                    return;
                }
                
                $scope.qosPreset = undefined;
                
                for ( var i = 0; angular.isDefined( $scope.presets ) && i < $scope.presets.length-1; i++ ) {
                    
                    var preset = $scope.presets[i];
                    
                    if ( $scope.qos.priority === preset.priority &&
                        $scope.qos.iopsMin === preset.iopsMin &&
                        $scope.qos.iopsMax === preset.iopsMax ){

                        $scope.qosPreset = preset;
                    }
                }
                
                if ( $scope.qosPreset === undefined ){
                    $scope.qosPreset = $scope.custom;
                    $scope.qosPreset.iopsMin = $scope.qos.iopsMin;
                    $scope.qosPreset.iopsMax = $scope.qos.iopsMax;
                    $scope.qosPreset.priority = $scope.qos.priority;
                }
            };
            
            var init = function(){

                $volume_api.getQosPolicyPresets( function( presets ){
                    
                    $scope.presets = presets;
                    $scope.presets.push( $scope.custom );
                    
                    rationalizeWithPresets();
                });
                
                
                refreshSliders();
            };
            
            $scope.limitSliderLabel = function( value ){

                if ( value === 0 ){
                    return '\u221E';
                }

                return value;
            };
            
            $scope.guaranteeSliderLabel = function( value ){
                
                if ( value === 0 ){
                    return $filter( 'translate' )( 'common.l_none' );
                }
                
                return value;
            };
            

            $scope.$on( 'fds::page_shown', refreshSliders);
            
            $scope.$on( 'fds::qos-reinit', init );
            
            $scope.$watch( 'qosPreset', function( newVal, oldVal ){
                
                if ( newVal === oldVal || !angular.isDefined( newVal ) ){
                    return;
                }

                $scope.qos = newVal;
            });
            
            
            $scope.$watch( 'qos.iopsMax', function(newVal, oldVal){
                
                if ( newVal !== 0 && newVal < $scope.qos.iopsMin && $scope.qos.iopsMin !== 0 ){
                    
                    var index = $scope.guaranteeChoices.length-2;
                    
                    while( index >= 0 && $scope.guaranteeChoices[index] > newVal ){
                        index--;
                    }
                    
                    $scope.qos.iopsMin = $scope.guaranteeChoices[index];
                    
                    refreshSliders();
                }
            });
            
            $scope.$watch( 'qos.iopsMin', function( newVal, oldVal ){
                
                if ( newVal !== 0 && newVal > $scope.qos.iopsMax && $scope.qos.iopsMax !== 0 ){
                    // go down the list until one is less
                    var index = 0;

                    while( index < $scope.limitChoices.length && $scope.limitChoices[index] < newVal ){
                        index++;
                    }

                    $scope.qos.iopsMax = $scope.limitChoices[index];

                    refreshSliders();
                }
            });
         
            init();
        }
    };
    
});