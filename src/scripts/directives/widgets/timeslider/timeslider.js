angular.module( 'form-directives' ).directive( 'timeSlider', function(){
    
    return {
        
        restrict: 'E',
        replace: true,
        transclude: false,
        templateUrl: 'scripts/directives/widgets/timeslider/timeslider.html',
        // validRanges = [ { min: #, max: #, label '', width: <special case that is considered if this range is the max to ensure scrubbing UX> } ... ]
        // we will add startPosition and endPosition to that range
        // domainLabels = [ {text: '', value: #}, ... ]
        scope: { validRanges: '=', selectedValue: '=ngModel', labelFunction: '=?', label: '@', domainLabels: '=?' },
        controller: function( $scope, $element, $document, $resize_service, $timeout ){
            
            var padding = 8;
            var min;
            var max;
            var halfHandleWidth = 5;
            
            $scope.editing = false;
            $scope.ratio = 1;
            $scope.sliderPosition = 0;
            $scope.grabbedSlider = false;
            $scope.filteredValue = '';
            var a = 0;
            var b = 0;
            var divisor = 100000000000;
            var power = 4;
            var endRangeWidth = 0;
            var endRange;
            
            var getWidth = function(){
                
                return $element.width() - endRangeWidth - 30;
            };
            
            // takes a value and maps it to a particular px
            // This equation is not a straight linear ratio type equation,
            // in order to change the distribution to have more "room" for recent
            // items it's currently executing a 4 degree polynomial function.
            // In the future we could choose to expose the curvature control and this
            // could become an nth degree function
            var valueToPosition = function( value ){
                
                if ( angular.isDefined( endRange ) && value > endRange.min ){
                    var ratio = (endRange.max - endRange.min) / endRangeWidth;
                    var pos = ((value - endRange.min) / ratio) + getWidth();
                    return pos;
                }
                
                value = value / divisor;
                
                var pos = a * Math.pow( value - min, power );
                
                return pos;
            };
            
            // takes a position in px and turns it into a value from our value range.
            var positionToValue = function( position ){
                
                if ( angular.isDefined( endRange ) && position > getWidth() ){
                    var ratio = (endRange.max - endRange.min) / endRangeWidth;
                    var value = ratio * (position - getWidth() ) + endRange.min;
                    return value;
                }
                
                var value = Math.pow( position / a, 1/power ) + min;
                return value;
            };
            
            var placeDomainLabels = function(){
                
                if ( !angular.isDefined( $scope.domainLabels ) ){
                    return;
                }
                
                for ( var i = 0; i < $scope.domainLabels.length; i++ ){
                    
                    var label = $scope.domainLabels[i];
                    var pos = valueToPosition( label.value );
                    var textWidth = measureText( label.text, 11 ).width;
                    
                    label.position = pos;
                    label.width = textWidth;
                    label.show = true;
                    
                    for ( var j = 0; j < i; j++ ){
                        var otherLabel = $scope.domainLabels[j];
                        
                        if ( isNaN( label.position ) || (label.position > otherLabel.position && label.position < otherLabel.width + otherLabel.position) ){
                            label.show = false;
                            break;
                        }
                        
                        if ( label.position < otherLabel.position && label.position + label.width > otherLabel.position ){
                            label.show = false;
                            break;
                        }
                    }
                }
            };
            
            var calculateInitializationSettings = function(){
                
                var minRange;
                var maxRange; 
                min = undefined;
                max = undefined;
                
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
                        minRange = val;
                    }
                    
                    if ( !angular.isDefined( max ) || val.max > max ){
                        max = val.max;
                        maxRange = val;
                    }
                }
                
                max = max / divisor;
                min = min / divisor;
                
                // special allowence if the max is encompassed in a range with a width - could do the same for the min but won't right now.
                if ( angular.isDefined( maxRange ) && angular.isDefined( maxRange.pwidth ) && (maxRange.max  - maxRange.min) > 0 ){
                    endRangeWidth = $element.width() * (maxRange.pwidth / 100.0);
                    endRange = maxRange;
                }
                
                a = getWidth() / Math.pow( max - min, power );
                
                for ( var j = 0; j < $scope.validRanges.length; j++ ){
                    var val = $scope.validRanges[j];
                    
                    // now calculate where it goes.
                    val.startPosition = valueToPosition( val.min );
                    
                    // if it's the special ending range
                    if ( (val.max/divisor) === max && (val.max - val.min) > 0 ){
                        val.startPosition = getWidth();
                        val.width = endRangeWidth;
                    }
                    else if ( val.max - val.min > 0 ){
                        var maxPos = valueToPosition( val.max );
                        var minPos = valueToPosition( val.min );
                        val.width = maxPos - minPos;
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

                if ( displayWidth + $scope.sliderPosition > getWidth() + endRangeWidth ){
                    return ($scope.sliderPosition - ((displayWidth + $scope.sliderPosition) - (getWidth() + endRangeWidth)) + 3*halfHandleWidth) + 'px';
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
            
            $scope.outOfRange = function( value ){
                
                value = value / divisor;
                return ( value > max || value < min );
            };
            
            // this will find the actual placement for the slider, then determine which existing point is closer
            // and snap to that position instead.
            $scope.snapToValidPoint = function( coordinate ){
                
                var vals = { position: undefined, value: undefined };
                var nearest;
                
                // find the closest valid tick mark
                for ( var i = 0; i < $scope.validRanges.length; i++ ){
                    
                    var range = $scope.validRanges[i];
                    var dist = Math.abs( range.startPosition - coordinate );
                    
                    var oDist = Math.abs( valueToPosition( range.max ) - coordinate );
                
                    // if it's within a range it is good to go
                    if ( angular.isDefined( range.width ) && range.width !== 2 && dist <= range.width && coordinate > range.startPosition ){
                        nearest = undefined;
                        vals.position = coordinate;
                        break;
                    }
                    
                    if ( !angular.isDefined( nearest ) || dist < nearest.distance ){
                        nearest = { range: range, distance: dist, position: range.startPosition, value: range.min };
                    }
                    
                    // incase we actually need to snap to the range end
                    if ( oDist < nearest.distance ){
                        nearest = { range: range, distance: dist, position: valueToPosition( range.max ), value: range.max };
                    }
                }
                
                if ( angular.isDefined( nearest ) ){
                    vals.position = nearest.position - halfHandleWidth;
                    vals.value = nearest.value;
                }
                // this is assuming the only time this is true is with an end range.
                // if we add the same exception for the minimum range, then this will need to change
                else if ( angular.isDefined( range ) ){
                    var ratio = (range.max - range.min) / endRangeWidth;
                    vals.value = ratio * (coordinate - getWidth() ) + range.min;
                }
                
                return vals;
            };
            
            $scope.snapToValidPointByValue = function( value ){
                return $scope.snapToValidPoint( valueToPosition( value ) );
            };
            
            $scope.sliderMoved = function( $event ){
                
                if ( $event.type === 'mousemove' && $scope.grabbedSlider !== true ){
                    return;
                }
                
                var coordinate = getEventX( $event );
                
                var nearest = $scope.snapToValidPoint( coordinate );
                
                $scope.selectedValue = nearest.value;
                $scope.sliderPosition = nearest.position;
                
                $timeout( function(){$scope.$apply();} );
            };
            
            var init = function(){
                calculateInitializationSettings();
                placeDomainLabels();
            };
            
            $document.on( 'mousemove', null, $scope.sliderMoved );
            $document.on( 'mouseup', null, handleReleased );
            
            $scope.$on( '$destroy', function(){
                $document.off( 'mousemove', null, $scope.sliderMoved );
                $document.off( 'mouseup', null, handleReleased );
            });
            
            $scope.$watch( 'validRanges', function(){
                init();
            });
            
            $scope.$watch( 'selectedValue', function( newVal ){
                
                if ( angular.isFunction( $scope.labelFunction ) ){
                    $scope.filteredValue = $scope.labelFunction( newVal );
                }
                else {
                    $scope.filteredValue = newVal;
                }
                
                var details = $scope.snapToValidPointByValue( newVal );
                $scope.sliderPosition = details.position;
                return details.value;
            });
            
            $scope.$on( 'fds::timeslider-refresh', function(){
                $timeout( function(){ 
                    init(); 
                    var details = $scope.snapToValidPointByValue( $scope.selectedValue );
                    $scope.sliderPosition = details.position;
                }, 50 );
            });
            
            $scope.$watch( 'domainLabels', init );
            
            init();
            
            $resize_service.register( $scope.$id, init );
        }
    };
});