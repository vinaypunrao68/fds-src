angular.module( 'form-directives' ).directive( 'spinner', function(){

    return {
        restrict: 'E',
        replace: true,
        transclude: false,
        scope: { value: '=', min: '=', max: '=', step: '=?', showButtons: '@', enable: '=?' },
        templateUrl: 'scripts/directives/widgets/spinner/spinner.html',
        controller: function( $scope, $timeout, $interval ){

            $scope.upPressed = false;
            $scope.downPressed = false;
            $scope.autoPress = {};
            
            var typing = false;
            
            if ( !angular.isDefined( $scope.step ) ){
                $scope.step = 1;
            }
            
            if ( !angular.isDefined( $scope.enable ) ){
                $scope.enable = true;
            }

            var cancelAutoPress = function(){
                if ( angular.isDefined( $scope.autoPress ) ){
                    $interval.cancel( $scope.autoPress );
                }
            };
            
            var correctMaxLength = function(){
                //determine maxlength
                $scope.maxlength = 1;

                var dividend = $scope.max;

                while( (dividend = (dividend / 10)) >= 1 ){
                    $scope.maxlength++;
                }
            };
            
            var init = function(){
                $scope.upPressed = false;
                $scope.downPressed = false;
                $scope.autoPress = {};

                if ( !angular.isDefined( $scope.value ) || $scope.value < $scope.min ){
                    $scope.value = $scope.min;
                }

                if ( !angular.isDefined( $scope.showButtons ) || $scope.showButtons === 'true' ){
                    $scope.buttons = true;
                }
                else {
                    $scope.buttons = false;
                }

                if ( $scope.value > $scope.max ){
                    $scope.value = $scope.max;
                }
                
                correctMaxLength();
            };

            $scope.incrementDown = function(){

                if ( $scope.enable === false ){
                    return;
                }
                
                $scope.upPressed = true;

                $timeout( function(){
                    // still pressed
                    if ( $scope.upPressed === true ){
                        $scope.autoPress = $interval( $scope.increment, 100 );
                    }
                },
                300 );
            };

            $scope.decrementDown = function(){

                if ( $scope.enable === false ){
                    return;
                }
                
                $scope.downPressed = true;

                $timeout( function(){
                    // still pressed
                    if ( $scope.downPressed === true ){
                        $scope.autoPress = $interval( $scope.decrement, 100 );
                    }
                },
                300 );
            };

            $scope.incrementUp = function(){

                if ( $scope.enable === false ){
                    return;
                }
                
                $scope.upPressed = false;

                cancelAutoPress();
            };

            $scope.decrementUp = function(){

                if ( $scope.enable === false ){
                    return;
                }
                
                $scope.downPressed = false;

                cancelAutoPress();
            };
            
            $scope.sendEvent = function(){
                $scope.$emit( 'change' );
                $scope.$emit( 'fds::spinner_change', $scope.value );
            };
            
            $scope.fixValue = function(){
                
                if ( $scope.value > $scope.max ){
                    $scope.value = $scope.max;
                    $scope.sendEvent();
                }
                else if ( $scope.value < $scope.min ){
                    $scope.value = $scope.min;
                    $scope.sendEvent();
                }
            };

            $scope.increment = function(){

                var val = parseInt( $scope.value );
                var step = parseInt( $scope.step );

                if ( $scope.value + $scope.step > $scope.max ){
                    $scope.value = $scope.max;
                }
                else {
                    $scope.value = val + step;
                }

                $scope.sendEvent();
            };

            $scope.decrement = function(){

                var val = parseInt( $scope.value );
                var step = parseInt( $scope.step );

                if ( $scope.value - $scope.step < $scope.min ){
                    $scope.value = $scope.min;
                }
                else {
                    $scope.value = val - step;
                }

                $scope.sendEvent();
            };
            
            $scope.keyPressed = function( $event ){
                
                if ( $scope.enable === false ){
                    return;
                }
                
                if ( $event.keyCode === 38 ){
                    $scope.increment();
                }
                else if ( $event.keyCode === 40 ){
                    $scope.decrement();
                }
                else {
                    typing = true;
                }
            };
            
            $scope.focusLost = function(){
                typing = false;
                $scope.fixValue();
                $scope.sendEvent();
            };
            
            $scope.$watch( 'value', function(){
                
                if( typing === true ){
                    return;
                }
                
                $scope.fixValue();
                $scope.sendEvent();
            });
            $scope.$watch( 'min', function(){
                correctMaxLength();
                $scope.fixValue();
            });
            $scope.$watch( 'max', function(){
                correctMaxLength();
                $scope.fixValue();
            });
            
            init();
        },
        link: function( $scope, $element, $attrs ){

            if ( !angular.isDefined( $attrs.step ) ){
                $attrs.step = 1;
            }

            if ( typeof $attrs.step !== 'number' ){
                $attrs.step = parseInt( $attrs.step );
            }

            if ( angular.isDefined( $attrs.max ) && typeof $attrs.max !== 'number' ){
                $attrs.max = parseInt( $attrs.max );
            }

            if ( angular.isDefined( $attrs.min ) && typeof $attrs.min !== 'number' ){
                $attrs.min = parseInt( $attrs.min );
            }

            if ( angular.isDefined( $attrs.value ) && typeof $attrs.value !== 'number' ){
                $attrs.value = parseInt( $attrs.min );
            }
        }
    };

});
