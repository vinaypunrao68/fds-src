angular.module( 'form-directives' ).directive( 'spinner', function(){

    return {
        restrict: 'E',
        replace: true,
        transclude: false,
        scope: { value: '=', min: '@', max: '@', step: '@' },
        templateUrl: 'scripts/directives/widgets/spinner/spinner.html',
        controller: function( $scope, $timeout, $interval ){

            $scope.upPressed = false;
            $scope.downPressed = false;
            $scope.autoPress = {};

            if ( !angular.isDefined( $scope.value ) || $scope.value < $scope.min ){
                $scope.value = $scope.min;
            }

            if ( $scope.value > $scope.max ){
                $scope.value = $scope.max;
            }

            //determine maxlength
            $scope.maxlength = 1;

            var dividend = $scope.max;

            while( dividend = (dividend / 10) >= 1 ){
                $scope.maxlength++;
            }

            var cancelAutoPress = function(){
                if ( angular.isDefined( $scope.autoPress ) ){
                    $interval.cancel( $scope.autoPress );
                }
            };

            $scope.incrementDown = function(){

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

                $scope.upPressed = false;

                cancelAutoPress();
            };

            $scope.decrementUp = function(){

                $scope.downPressed = false;

                cancelAutoPress();
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

                $scope.$emit( 'change' );
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

                $scope.$emit( 'change' );
            };

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
