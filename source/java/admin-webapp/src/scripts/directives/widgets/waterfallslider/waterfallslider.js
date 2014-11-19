angular.module( 'form-directives' ).directive( 'waterfallSlider', function(){
    
    return {
        restrict: 'E',
        replace: true,
        transclude: false,
        templateUrl: 'scripts/directives/widgets/waterfallslider/waterfallslider.html',
        // format:  sliders = [{value: val, name: name},...]
        // range: [{start: val, stop: val, values: []}..]
        scope: { sliders: '=', range: '=' },
        controller: function( $scope, $document, $element ){
            
            $scope.grabbedSlider = undefined;
            $scope.sliderIndex = 0;
            
            var halfHandleWidth = 5;
            
            var init = function(){
                
                for ( var i = 0; i < $scope.sliders.length; i++ ){
                    $scope.sliders[i].position = i * Math.floor( $element[0].offsetWidth / $scope.sliders.length ) - halfHandleWidth;
                }
            };
            
            getEventX = function( $event ){
                var relativeX = ($event.clientX - $element.offset().left) - halfHandleWidth;
                return relativeX;
            };
            
            $scope.handleClicked = function( slider, $index ){
                $scope.grabbedSlider = slider;
                $scope.sliderIndex = $index;
            };
            
            var handleReleased = function( $event ){
                $scope.grabbedSlider = undefined;
                $scope.$apply();
            };
            
            var rationalizePositionWithRange = function( pos ){
                
                var bucketSize = Math.floor( $element[0].offsetWidth / $scope.range.length );
                var rangeIndex = Math.floor( pos / bucketSize );
                
                var zero = rangeIndex * bucketSize;
                
                var pxRemaining = pos - zero;
                
                var pxInBucket = Math.floor( bucketSize / ($scope.range[rangeIndex].end - $scope.range[rangeIndex].start));
                
                pos = Math.round( pos / pxInBucket ) * pxInBucket;
                
                return pos;
            };
            
            var sliderMoved = function( $event ){
                
                if ( !angular.isDefined( $scope.grabbedSlider ) ){
                    return;
                }
                
                var pos = getEventX( $event );
                
                if ( pos < -1*halfHandleWidth ){
                    pos = -1*halfHandleWidth;
                }
                else if ( pos > $element[0].offsetWidth+halfHandleWidth ){
                    pos = $element[0].offsetWidth+halfHandleWidth;
                }
                else if ( $scope.sliderIndex != 0 && pos < $scope.sliders[ $scope.sliderIndex - 1 ].position ){
                    pos = $scope.sliders[ $scope.sliderIndex - 1 ].position;
                }
                
                pos = rationalizePositionWithRange( pos );
                
                $scope.grabbedSlider.position = pos;
                
                for ( var i = $scope.sliderIndex+1; i < $scope.sliders.length; i++ ){
                    if ( pos > $scope.sliders[i].position ){
                        $scope.sliders[i].position = pos;
                    }
                }
            
                $scope.$apply();
            };
            
            $document.on( 'mousemove', null, sliderMoved );
            $document.on( 'mouseup', null, handleReleased );

            $scope.$on( 'destroy', function(){
                $document.off( 'mousemove', null, sliderMoved );
                $document.off( 'mouseup', null, handleReleased );
            });
            
            init();
        }
    };
    
});