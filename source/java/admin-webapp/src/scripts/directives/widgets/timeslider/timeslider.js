angular.module( 'form-directives' ).directive( 'timeSlider', function(){
    
    return {
        
        restrict: 'E',
        replace: true,
        transclude: false,
        templateUrl: 'scripts/directives/widgets/timeslider/timeslider.html',
        // validRanges = [ { min: #, max: #, label '' } ... ]
        // we will add startPosition and endPosition to that range
        scope: { validRanges: '=', selectedValue: '=ngModel', labelFunction: '=?', label: '@' },
        controller: function( $scope, $element, $document, $resize_service, $timeout ){
            
            var padding = 8;
            var min;
            var max;
            var halfHandleWidth = 5;
            
            $scope.ratio = 1;
            $scope.sliderPosition = 0;
            $scope.grabbedSlider = false;
            $scope.filteredValue = '';
            
            var getWidth = function(){
                return $element.width() - (2*padding);
            };
            
            var calculateScale = function(){
                
                for ( var i = 0; i < $scope.validRanges.length; i++ ){
                    var val = $scope.validRanges[i];
                    
                    if ( !angular.isDefined( val.min ) ){
                        val.min = val.max;
                    }
                    
                    if ( !angular.isDefined( val.max ) ){
                        val.max = val.min;
                    }
                    
                    if ( !angular.isDefined( val.min ) && !angular.isDefined( val.max ) ){
                        continue;
                    }
                    
                    if ( !angular.isDefined( min) || val.min < min ){
                        min = val.min;
                    }
                    
                    if ( !angular.isDefined( max ) || val.max > max ){
                        max = val.max;
                    }
                }
                
                $scope.ratio = $element.width() / ( max - min );
                
                for ( var j = 0; j < $scope.validRanges.length; j++ ){
                    var val = $scope.validRanges[j];
                    
                    // now calculate where it goes.
                    val.startPosition = $scope.ratio * (val.min - min);
                    
                    if ( val.max - val.min > 0 ){
                        val.width = $scope.ratio * (val.max-val.min);
                    }
                    else {
                        val.width = 2;
                    }
                }
            };
            
            // helper function to get the relative x value in the element from the mouse event
            var getEventX = function( $event ){
                var relativeX = ($event.clientX - $element.offset().left) - halfHandleWidth;
                return relativeX;
            };
            
            $scope.placeMe = function(){
                var displayWidth = $($element.find( '.edit-box' )).width();
                
                if ( displayWidth + $scope.sliderPosition > getWidth() ){
                    return ($scope.sliderPosition - ((displayWidth + $scope.sliderPosition) - getWidth()) + 3*halfHandleWidth) + 'px';
                }
                else {
                    return ($scope.sliderPosition - 5) + 'px';
                }
            };
            
            $scope.handleClicked = function(){
                $scope.grabbedSlider = true;
            };
            
            var handleReleased = function(){
                $scope.grabbedSlider = false;
            };
            
            $scope.sliderMoved = function( $event ){
                
                if ( $event.type === 'mousemove' && $scope.grabbedSlider !== true ){
                    return;
                }
                
                var coordinate = getEventX( $event );
                var nearest;
                
                // find the closest valid tick mark
                for ( var i = 0; i < $scope.validRanges.length; i++ ){
                    
                    var range = $scope.validRanges[i];
                    var dist = Math.abs( range.startPosition - coordinate );
                    
                    // if it's within a range it is good to go
                    if ( angular.isDefined( range.width ) && range.width !== 2 && dist <= range.width ){
                        nearest = undefined;
                        $scope.sliderPosition = coordinate;
                        break;
                    }
                    
                    if ( !angular.isDefined( nearest ) || dist < nearest.distance ){
                        nearest = { range: range, distance: dist };
                    }
                }
                
                if ( angular.isDefined( nearest ) ){
                    $scope.sliderPosition = nearest.range.startPosition - halfHandleWidth;
                    $scope.selectedValue = nearest.range.min;
                }
                else {
                    $scope.selectedValue = ($scope.sliderPosition / $scope.ratio ) + min;
                }
                
                $timeout( function(){$scope.$apply();} );
            };
            
            var init = function(){
                calculateScale();
            };
            
            $document.on( 'mousemove', null, $scope.sliderMoved );
            $document.on( 'mouseup', null, handleReleased );
            
            $scope.$on( '$destroy', function(){
                $document.off( 'mousemove', null, $scope.sliderMoved );
                $document.off( 'mouseup', null, handleReleased );
            });
            
            $scope.$watch( 'selectedValue', function( newVal ){
                
                if ( angular.isFunction( $scope.labelFunction ) ){
                    $scope.filteredValue = $scope.labelFunction( newVal );
                }
                else {
                    $scope.filteredValue = newVal;
                }
            });
            
            init();
        }
    };
});