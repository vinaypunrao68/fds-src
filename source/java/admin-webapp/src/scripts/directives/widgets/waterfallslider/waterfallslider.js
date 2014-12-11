angular.module( 'form-directives' ).directive( 'waterfallSlider', function(){
    
    return {
        restrict: 'E',
        replace: true,
        transclude: false,
        templateUrl: 'scripts/directives/widgets/waterfallslider/waterfallslider.html',
        // format:  sliders = [{value: { range: index, value: value }}, name: name},...]
        // range: [{start: val, end: val (will not be displayed unless last range), values: [], width: <in percentage>, segments: <segments to draw>, min: minimum value (only applies to first item), labelFunction: callback for label, selectable: if it can be in the drop down, selectName: name in the dropdown,
        // allowNumber: hides or shows the spinner}..]
        scope: { sliders: '=', range: '=', enabled: '=' },
        controller: function( $scope, $document, $element, $timeout, $resize_service ){
            
            $scope.grabbedSlider = undefined;
            $scope.sliderIndex = 0;
            $scope.showAnimation = false;
            $scope.hover = -1;
            $scope.editing = -1;
            $scope.choices = [];
            $scope.spinnerMin = 1;
            $scope.spinnerMax = 99;
            $scope.spinnerStep = 1;
            $scope.spinnerValue = 1;
            $scope.dropdownRange = {};
            $scope.validPositions = [];
//            $scope.sliderPaneWidth = 0;
            $scope.allowNumber = true;
            
            var labelPane = {};
            var sliderPane = {};
            var halfHandleWidth = 5;
            
            $scope.amITooWide = function( $index ){
                var w = $($element.find( '.value-container')[$index]).width();
                
                if ( $scope.sliders[$index].position + w > sliderPane.width() /*$scope.sliderPaneWidth*/ ){
                    return true; 
                }
                
                return false;
            };
            
            /** get jQuery representations of our main objects so we can use their size to calculate valid stops **/
            var determinePanelWidths = function(){
                
                sliderPane = $($element.find( '.slider-pane' )[0]);
                labelPane = $($element.find( '.labels' )[0]);
                
//                $scope.sliderPaneWidth = sliderPane.width();
            };
            
            /**
            * Sometimes there isn't a one to one relationship between the sliders and what needs to be 
            * in the drop down so we need to manufacture that array here.
            **/
            var createDropdownChoices = function(){
                
                $scope.choices = [];
                
                for ( var i = 0; i < $scope.range.length; i++ ){
                    
                    // do not add it if selectable is false
                    if ( angular.isDefined( $scope.range[i].selectable &&
                        $scope.range[i].selectable === false ) ){
                        
                        continue;
                    }
                    
                    $scope.choices.push( $scope.range[i] );
                }
            };
            
            /** Helper to calculate how many pixels each segment in the range is worth **/
            var findPixelsPerRangeSegment = function( range, segments ){
                var rtn = Math.floor( ((range.width/100)*sliderPane.width())/segments );
                return rtn;
            };
            
            /** helper to determine the number of segments in a range **/
            var getSegmentsForRange = function( range ){
                var segments = range.end - range.start;
                
                if ( angular.isDefined( range.segments ) && angular.isNumber( range.segments ) ){
                    segments = range.segments;
                }
                
                return segments;
            };
            
            /**
            * This method looks at all the widths specified in the range items and divides the
            * remaining element width evenly over ranges where no width is specified.
            **/
            var fixRangeWidths = function(){
                
                var total = 0;
                var notSpecified = [];
                
                for( var i = 0; i < $scope.range.length; i++ ){
                    if ( angular.isDefined( $scope.range[i].width ) && angular.isNumber( $scope.range[i].width ) ){
                        total += $scope.range[i].width;
                    }
                    else {
                        notSpecified.push( $scope.range[i] );
                    }
                }
                
                if ( notSpecified.length > 0 && total < 100 ){
                    var leftOver = 100 - total;
                    var per = Math.floor( leftOver / notSpecified.length );
                    leftOver = 100 - (per*notSpecified.length);
                    
                    for ( var j = 0; j < notSpecified.length; j++ ){
                        notSpecified[j].width = per;
                    }
                    
                    var it = 0;
                    while( leftOver > 0 ){
                        notSpecified[it % notSpecified.length].width += 1;
                        leftOver -= 1;
                        it++;
                    }
                }
                else if ( total < 100 || total > 100 ){
                    var leftOver = 100 - total;
                    
                    var it = 0;
                    while( leftOver !== 0 ){
                        $scope.range[it % $scope.range.length].width += 1;
                        leftOver -= 1;
                        it++;
                    }
                }
            };
            
            /**
            * Need to catalogue all the valid positions so the maths are only calculated one time
            */
            var initializeValidPositions = function(){
                
                var xaxis = $($element.find('.waterfall-legend')[0]);
                var zero = 0;
                $scope.validPositions = [];
                
                for ( var rangeIt = 0; rangeIt < $scope.range.length; rangeIt++ ){
                    
                    var range = $scope.range[ rangeIt ];
                    var rangeWidth = (range.width / 100.0 ) * sliderPane.width();
                    
                    var segments = getSegmentsForRange( range );
                    var pxPer = findPixelsPerRangeSegment( range, segments );
                    
                    for ( var segIt = 0; segIt < segments; segIt++ ){
                        var pos = (segIt*pxPer) + zero;
                        
                        var value = Math.floor( (segIt * (range.end - range.start)/ segments) ) + range.start;
                        
                        if ( rangeIt === 0 && angular.isDefined( range.min ) && value < range.min ){
                            continue;
                        }
                        
                        $scope.validPositions.push( { position: pos, range: rangeIt, value:  value } );
                    }
                    
                    if ( rangeIt+1 !== $scope.range.length ){
                        zero += rangeWidth;
                    }
                }
                
                // gotta add the very last tick mark
                var range = $scope.range[ $scope.range.length-1 ];
                var pxPer = findPixelsPerRangeSegment( range, getSegmentsForRange( range ) );
                var pos = (getSegmentsForRange( range ) * pxPer) + zero;
                
                // this means there was only one value in the last range
                if ( isNaN( pos ) ){
                    pos = zero + rangeWidth - halfHandleWidth;
                }
                
                var value = range.end;
                
                $scope.validPositions.push( { position: pos, range: $scope.range.length-1, value: value } );
            };
            
            /**
            * Create the tick mark labels.  Put one at every range start.
            **/
            var createDomainLabels = function(){
                
                var labelDiv = $($element.find( '.waterfall-labels')[0]);
                
                for ( var rangeIt = 0; rangeIt < $scope.range.length; rangeIt++ ){
                    
                    // don't label the first one
                    if ( rangeIt === 0 ){
                        continue;
                    }
                    
                    var range = $scope.range[ rangeIt ];
                    
                    var pos = findPositionForValue( { value: range.start, range: rangeIt } ).position;
                    
                    var text = $scope.createLabel( {value: range.start, range: rangeIt} );
//                    var textWidth = measureText( text, 11 ).width;
//                    pos -= pos/2.0 - (2*halfHandleWidth);
                    
                    labelDiv.append( '<div class="waterfall-label" style="left: ' + pos + 'px">' + text + '</div>' );
                }
                
                // adding the final label
                var range = $scope.range[ $scope.range.length-1 ];
                var myVal = { value: range.end, range: $scope.range.length-1 };
                var pos = findPositionForValue( myVal ).position;
                var label = $scope.createLabel( myVal );
                labelDiv.append( '<div class="waterfall-label" style="left: ' + pos + 'px">' + label + '</div>' );
            };
            
            /**
            * Helper to determine whether the waterfall sliders are in an invalid state and correct them.
            * This stops sliders from going off the track, and enforces the waterfall drag behavior
            **/
            var fixStartPositions = function(){
                
                for ( var i = 0; i < $scope.sliders.length-1; i++ ){
                    
                    var lSlider = $scope.sliders[i];
                    var hSlider = $scope.sliders[i+1];
                    
                    if ( lSlider.position > hSlider.position ){
                        hSlider.position = lSlider.position;
                        hSlider.value = lSlider.value;
                        hSlider.startPosition = lSlider.position + halfHandleWidth;
                    }
                    else {
                        hSlider.startPosition = lSlider.position + halfHandleWidth;
                    }
                }
            };
            
            
            // helper function to get the relative x value in the element from the mouse event
            var getEventX = function( $event ){
                var relativeX = ($event.clientX - sliderPane.offset().left) - halfHandleWidth;
                return relativeX;
            };
        
            var rationalizePositionWithRange = function( pos ){
                
                for ( var i = 0; i < $scope.validPositions.length; i++ ){
                    var testPos = $scope.validPositions[i];
                    
                    var lastPos = -1;
                    
                    if ( i !== 0 ){
                        lastPos = $scope.validPositions[i-1];
                    }
                    
                    if ( pos < testPos.position ){
                        
                        // I'm less than the first valid position so... I am that position.
                        if ( i === 0 ){
                            pos = testPos;
                            break;
                        }
                        
                        // I'm at least at index 1 so I am between these two positions.
                        if ( (pos - lastPos.position) >= (testPos.position - pos ) ){
                            pos = testPos;
                        }
                        else {
                            pos = lastPos;
                        }
                        
                        break;
                    }
                }
                
                // must be off the screen
                if ( !angular.isDefined( pos.position ) ){
                    pos = $scope.validPositions[ $scope.validPositions.length - 1 ];
                }
                
                return { position: pos.position, value: { value: pos.value, range: pos.range } };
            };
            
            /**
            *  Sometimes we need to set the position but we only know the value
            *  whether it's at initialization time or being set by a field
            **/
            var findPositionForValue = function( value ){
                
                for( var i= 0; i < $scope.validPositions.length; i++ ){
                    
                    var testPos = $scope.validPositions[i];
                    
                    if ( value.range === testPos.range && value.value === testPos.value ){
                        
                        return { position: testPos.position, value: { value: testPos.value, range: testPos.range } };        
                    }
                }
            };
            
            $scope.sliderMoved = function( $event, slider ){
                
                if ( !angular.isDefined( slider ) ){
                    slider = $scope.grabbedSlider;
//                    $scope.showAnimation = false;
                }
                // if the slider is defined we need to make sure the slider index is correct
                else {
                    for ( var si = 0; si < $scope.sliders.length; si++ ){
                        if ( $scope.sliders[si] === slider ){
                            $scope.sliderIndex = si;
                            break;
                        }
                    }
                }
                
                if ( !angular.isDefined( slider ) ){
                    return;
                }
                
                var pos = getEventX( $event );
                
                if ( pos < -1*halfHandleWidth ){
                    pos = -1*halfHandleWidth;
                }
                else if ( pos > sliderPane.width()+halfHandleWidth ){
                    pos = sliderPane.width()+halfHandleWidth;
                }
                else if ( $scope.sliderIndex !== 0 && pos < $scope.sliders[ $scope.sliderIndex - 1 ].position ){
                    pos = $scope.sliders[ $scope.sliderIndex - 1 ].position;
                }
                
                var posVal = rationalizePositionWithRange( pos );
                pos = posVal.position;
                
                slider.value = posVal.value;
                slider.position = pos;
                
                fixStartPositions();

                $timeout( function(){ $scope.$apply();});
            };
            
            $scope.setSliderValue = function( slider ){
                
                for ( var i = 0; i < $scope.range.length; i++ ){
                    if ( $scope.range[i] === $scope.dropdownRange ){
                        slider.value.range = i;
                        break;
                    }
                }
                
                slider.value.value = $scope.spinnerValue;
                
                // if it's at the end of a range, bump it to the first of the next.
                if ( slider.value.value === $scope.range[slider.value.range].end && slider.value.range !== $scope.range.length - 1 ){
                    slider.value.range++;
                    slider.value.value = $scope.range[slider.value.range].start;
                }
                
                slider.position = findPositionForValue( slider.value ).position;
                fixStartPositions();
                
                $scope.editing = -1;
                
                $timeout( function(){ $scope.$apply();});
            };
            
            $scope.handleClicked = function( slider, $index ){
                $scope.grabbedSlider = slider;
                $scope.sliderIndex = $index;
                $scope.editing = -1;
            };
            
            var handleReleased = function( $event ){
                $scope.grabbedSlider = undefined;
                $timeout( function(){ $scope.$apply();});
            };
            
            $scope.createLabel = function( value ){
                
                var str = undefined;
                
                if ( angular.isFunction( $scope.range[value.range].labelFunction ) ){
                    str = $scope.range[value.range].labelFunction( value.value );
                }
                
                if ( !angular.isDefined( str ) ) {
                    str = value.value + ' ' + $scope.range[value.range].name;
                }
                
                return str;
            };
            
            $scope.sliderEdit = function( $index ){
                
                var slider = $scope.sliders[$index];
                var range = $scope.range[ slider.value.range ];
                
                $scope.editing = $index;
                $scope.dropdownRange = $scope.range[slider.value.range];
                $scope.spinnerValue = slider.value.value;
                $scope.spinnerMax = range.end;
                
                if ( slider.value.range !== $scope.range.length-1 ){
                    $scope.spinnerMax = range.end - 1;
                    
                    if ( angular.isDefined( range.step ) ){
                        $scope.spinnerMax = range.end - range.step;
                    }
                }
                
                $scope.spinnerMin = range.start;
                $scope.spinnerStep = Math.floor( (range.end-range.start)/getSegmentsForRange( range ) );
            };
            
            // initialize the widet.
            var init = function(){
                
                fixRangeWidths();  
                determinePanelWidths();
                createDropdownChoices();
                
                // needs second for the rendering to complete so that width calculations work properly.  Otherwise we're a variant number
                // of partial pixels off
                $timeout( 
                    function(){
                        var labelDiv = $($element.find( '.waterfall-labels')[0]);
                        labelDiv.empty();
                        
                        initializeValidPositions();
                        createDomainLabels();
                        
                        // set to the right values.
                        for( var i = 0; i < $scope.sliders.length; i++ ){

                            if ( !angular.isDefined( $scope.sliders[i].value ) ){
                                $scope.sliders[i].value = { range: 0, value: $scope.range[0].start };
                            }

                            var posObj = findPositionForValue( $scope.sliders[i].value );
                            
                            if ( !angular.isDefined( posObj ) ){
                                continue;
                            }
                            
                            $scope.sliders[i].position = posObj.position;
                        }
                        
                        fixStartPositions();
                    }, 50 );
            };
            
            $document.on( 'mousemove', null, $scope.sliderMoved );
            $document.on( 'mouseup', null, handleReleased );
            
            $scope.$on( 'fui::dropdown_change', function( $event, range ){
                $scope.spinnerMin = range.start;
                $scope.spinnerMax = range.end;
                
                if ( range !== $scope.range.length-1 ){
                    $scope.spinnerMax = range.end - 1;
                    
                    if ( angular.isDefined( range.step ) ){
                        $scope.spinnerMax = range.end - range.step;
                    }
                }
                
                $scope.spinnerStep = Math.floor( (range.end - range.start) / getSegmentsForRange( range ));
                
                if ( angular.isDefined( range.allowNumber ) ){
                    $scope.allowNumber = range.allowNumber;
                }
                else {
                    $scope.allowNumber = true;
                }
                
                $scope.dropdownRange = range;
            });
            
            $scope.$on( 'fds::spinner_change', function( $event, value ){
                
                if ( value === $scope.spinnerValue ){
                    return;
                }
                
                $scope.spinnerValue = parseInt( value ); 
            });

            $scope.$on( '$destroy', function(){
                $document.off( 'mousemove', null, $scope.sliderMoved );
                $document.off( 'mouseup', null, handleReleased );
            });

            /**
            * This is a crazy watch that looks at anything in the sliders array that may change.
            * We are concerned if the value object in each slider changes because if it does,
            * we need to move the slider appropriately
            **/
            var unwatch = $scope.$watch( 'sliders', function( newList, oldList ){
                
                // the lists need to be the same length and neither should be null.
                // we only want this to happen on changes - not initializations.
                if ( newList.length !== oldList.length || !angular.isDefined( newList ) || !angular.isDefined( oldList ) ){
                    return;
                }
                
                for ( var it = 0; it < newList.length; it++ ){
                    
                    var newSlider = newList[it];
                    var oldSlider = oldList[it];
                    
                    if ( newSlider.value.value == oldSlider.value.value &&
                        newSlider.value.range == oldSlider.value.range ){
                        continue;
                    }
                    
                    // the value changed - recalculate it
                    var posObj = findPositionForValue( newSlider.value );
                    
                    if ( !angular.isDefined( posObj ) ){
                        continue;
                    }
                    
                    newSlider.position = posObj.position;
                }
                
                fixStartPositions();
                
            }, true);
            
            init();
            $resize_service.register( $scope.$id, init );
        }
    };
    
});