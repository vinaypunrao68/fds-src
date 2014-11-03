angular.module( 'angular-fui' ).directive( 'fuiSlider', function(){

    return {
        restrict: 'E',
        replace: true,
        transclude: false,
        scope: { min: '@', max: '@', value: '=ngModel', minLabel: '@', maxLabel: '@', labelFunction: '=?', step: '@', values: '=' },
        templateUrl: 'scripts/directives/angular-fui/fui-slider/fui-slider.html',
        controller: function( $scope, $document, $element ){

            $scope.position = 0;
            $scope.grabbed = false;
            $scope.segments = [];

            var initBounds = function(){
                if ( !angular.isDefined( $scope.min ) && angular.isDefined( $scope.values ) ){
                    $scope.min = 0;
                    $scope.max = $scope.values.length - 1;
                    $scope.step = 1;
                }
            };

            var getPxPerStep = function(){
                var eWidth = $element[0].clientWidth;

                if (!angular.isDefined( $scope.max ) || !angular.isDefined( $scope.min ) ){
                    initBounds();
                }

                var totalSteps = Math.round( ($scope.max - $scope.min) / $scope.step );
                var v = Math.ceil( eWidth / totalSteps );

                $scope.segments = [];
                for( var i = 0; i < totalSteps-1 && v > 10; i++ ){
                    $scope.segments.push( {id: i, px: v} );
                }

                return v;
            };

            $scope.getClickX = function( $event ){
                return ($event.clientX - $element.offset().left);
            };

            $scope.adjustSlider = function( clickX ){

                initBounds();

                var eWidth = $element[0].clientWidth;
                var pxPerStep = getPxPerStep();
                var steps = Math.round( clickX / pxPerStep );

                var shift = pxPerStep * steps;

                if ( shift > eWidth ){
                    shift = eWidth;
                }

                if ( shift < 0 ){
                    shift = 0;
                }

                $scope.position = shift;
                var foundValue = 0;

                if ( angular.isDefined( $scope.values ) ){
                    foundValue =  $scope.values[steps];
                }
                else {
                    foundValue = $scope.min + (steps * $scope.step);
                }

                // keep it in bounds
                if ( foundValue > $scope.max ){
                    $scope.value = $scope.max;
                }
                else if ( foundValue < $scope.min ){
                    $scope.value = $scope.min;
                }
                else {
                    $scope.value = foundValue;
                }
            };

            $scope.mouseMoved = function( $event ){

                if ( $scope.grabbed !== true ){
                    return;
                }

                $event.stopPropagation();
                $event.preventDefault();

                $scope.adjustSlider( $scope.getClickX( $event ) );
                $scope.$apply();
            };

            $scope.setValue = function(){

                var pxPerStep = getPxPerStep();

                if ( isNaN( pxPerStep ) || pxPerStep === 0 ){
                    return;
                }

                var steps = 0;

                if ( angular.isDefined( $scope.values ) ){

                    for ( steps = 0; steps < $scope.values.length; steps++ ){
                        if ( $scope.values[steps] === $scope.value ){
                            break;
                        }
                    }
                }
                else {
                    steps = Math.round( $scope.value / $scope.step );
                    $scope.value = $scope.min + ( steps * $scope.step );
                }

                $scope.position = pxPerStep * steps;
            };

            $document.on( 'mousemove', null, $scope.mouseMoved );
            $document.on( 'mouseup', null, function(){ $scope.grabbed = false; });

            $scope.$on( 'destroy', function(){
                $document.off( 'mousemove', null, $scope.mouseMoved );
                $document.off( 'mouseup', null, function(){ $scope.grabbed = false; });
            });

            $scope.$on( 'fds::fui-slider-refresh', $scope.setValue );

            $scope.setValue();
        },
        link: function( $scope, $element, $attrs ){

            if (!angular.isDefined( $attrs.step ) ){
                $attrs.step = 1;
            }

            if ( angular.isDefined( $attrs.min ) && typeof $attrs.min !== 'number' ){
                $attrs.min = parseInt( $attrs.min );
            }

            if ( angular.isDefined( $attrs.max ) && typeof $attrs.max !== 'number' ){
                $attrs.max = parseInt( $attrs.max );
            }
        }
    };
});
