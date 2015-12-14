angular.module( 'form-directives' ).directive( 'slider', function(){

    return {
        restrict: 'E',
        replace: true,
        transclude: false,
        scope: { min: '@', max: '@', value: '=ngModel', minLabel: '@', maxLabel: '@', labelFunction: '=?', step: '@', values: '=' },
        templateUrl: 'scripts/directives/widgets/slider/slider.html',
        controller: function( $scope, $document, $element, $timeout ){

            $scope.position = 0;
            $scope.grabbed = false;
            $scope.segments = [];

            var initBounds = function(){
                if ( angular.isDefined( $scope.values ) ){
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

                var totalSteps = Math.round( (parseInt( $scope.max ) - parseInt( $scope.min ) ) / parseInt( $scope.step ) );
                var v = Math.ceil( eWidth / totalSteps );

                $scope.segments = [];
                for( var i = 0; i < totalSteps && v > 10; i++ ){
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
                    $scope.value =  $scope.values[steps];
                    return;
                }
                else {
                    foundValue = parseInt( $scope.min ) + (steps * parseInt( $scope.step ) );
                }

                // keep it in bounds
                if ( foundValue > parseInt( $scope.max ) ){
                    $scope.value = parseInt( $scope.max );
                }
                else if ( foundValue < parseInt( $scope.min ) ){
                    $scope.value = parseInt( $scope.min );
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
                    steps = Math.round( ($scope.value - parseInt( $scope.min ) ) / parseInt( $scope.step ) );
                    $scope.value = parseInt( $scope.min ) + ( steps * parseInt( $scope.step ) );
                }

                $scope.position = (pxPerStep * (steps));
            };
            
            $scope.getLabel = function( $index ){
                
                var val = $scope.getIndexValue( $index );
                
                if ( angular.isFunction( $scope.labelFunction ) ){
                    return $scope.labelFunction( val  );
                }
                
                return val;
            };
            
            $scope.getLeftForSegmentLabel = function( segment ){
                
                // we don't want weird hangovers so the first label will be at 0px
                if ( segment.id === 0 ){
                    return 0;
                }
                
                var size = measureText( $scope.getLabel( segment.id ), 11 );
                
                // the 2 here is for the padding...
                var pos = (segment.id * segment.px) - ( size.width / 2 ) - 2;
                
                return pos;
            };
            
            $scope.getIndexValue = function( $index ){
                if ( angular.isDefined( $scope.values ) ){
                    return $scope.values[ $index ];
                }
                else {
                    return parseInt( $index ) * parseInt( $scope.step ) + parseInt( $scope.min );
                }
            };

            $document.on( 'mousemove', null, $scope.mouseMoved );
            $document.on( 'mouseup', null, function(){ $scope.grabbed = false; });

            $scope.$on( 'destroy', function(){
                $document.off( 'mousemove', null, $scope.mouseMoved );
                $document.off( 'mouseup', null, function(){ $scope.grabbed = false; });
            });

            var asyncSetValue = function(){
                $timeout( $scope.setValue, 100 );
            };
            
            $scope.$on( 'fds::page_shown', asyncSetValue );
            $scope.$on( 'fds::fui-slider-refresh', asyncSetValue );

            asyncSetValue();
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
