angular.module( 'form-directives' ).directive( 'waterfallSlider', function(){
    
    return {
        restrict: 'E',
        replace: true,
        transclude: false,
        templateUrl: 'scripts/directives/widgets/waterfallslider/waterfallslider.html',
        // format:  sliders = [{value: { range: index, value: value }}, name: name},...]
        // range: [{start: val, end: val (will not be displayed unless last range), values: [], width: <in percentage>, segments: <segments to draw>}..]
        scope: { sliders: '=', range: '=' },
        controller: function( $scope, $document, $element, $timeout, $resize_service ){
            
            $scope.grabbedSlider = undefined;
            $scope.sliderIndex = 0;
            $scope.showAnimation = false;
            $scope.hover = -1;
            $scope.editing = -1;
            
            var labelPane = {};
            var sliderPane = {};
            var halfHandleWidth = 5;
            
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
            
            var determinePanelWidths = function(){
                
                sliderPane = $($element.find( '.slider-pane' )[0]);
                labelPane = $($element.find( '.labels' )[0]);
            };
            
            // helper function to get the relative x value in the element from the mouse event
            var getEventX = function( $event ){
                var relativeX = ($event.clientX - sliderPane.offset().left) - halfHandleWidth;
                return relativeX;
            };
        
            var rationalizePositionWithRange = function( pos ){
                
                var zero = 0;
                var range;
                
                // find which range we're in
                for ( var i = 0; i < $scope.range.length; i++ ){
                    var width = ($scope.range[i].width / 100) * sliderPane.width();
                    
                    if ( pos < (width + zero) || (i+1) === $scope.range.length ){
                        range = $scope.range[i];
                        break;
                    }
                    
                    zero += width;
                }
                
                // find which tick mark we're at.
                var segments = range.end - range.start;
                
                if ( angular.isDefined( range.segments ) && angular.isNumber( range.segments ) ){
                    segments = range.segments;
                }
                
                var pxPer = Math.floor( ((range.width/100)*sliderPane.width())/segments );
                
                var step = Math.round( (pos-zero) / pxPer );
                
                // we don't want to show the last value, instead we show the first value of the next segment unless it's the last one
                if ( step === segments && range !== $scope.range[ $scope.range.length-1 ] ){
                    pos = zero + (range.width/100)*sliderPane.width();
                    return rationalizePositionWithRange( pos );
                }
                
                pos = step * pxPer + zero;
                
                if ( pos > sliderPane.width() - halfHandleWidth ){
                    pos = sliderPane.width() - halfHandleWidth;
                }
                
                var value = (((range.end - range.start) / segments) * step) + range.start;
                
                return { position: pos, value: { value: value, range: i } };
            };
            
            /**
            *  Sometimes we need to set the position but we only know the value
            *  whether it's at initialization time or being set by a field
            **/
            var findPositionForValue = function( value ){
                
                var zero = 0;
                
                // special boundary case on the for right
                if ( value.value === $scope.range[ value.range ].end ){
                    
                    // if there's another range just set this to the first value in that range
                    if ( value.range < $scope.range.length-1 ){
                        value.range++;
                        value.value = $scope.range[ value.range ].start;
                    }
                }
                
                for ( var i = 0; i < value.range; i++ ){
                    zero += ($scope.range[i].width/100) * sliderPane.width();
                }
                
                var myRange = $scope.range[ value.range ];
                
                // find which tick mark we're at.
                var segments = myRange.end - myRange.start;
                
                if ( angular.isDefined( myRange.segments ) && angular.isNumber( myRange.segments ) ){
                    segments = myRange.segments;
                }
                
                var whichSegment = Math.round( value.value / ((myRange.end - myRange.start) / segments)) - 1;
                var pxPerSegment = Math.floor( ((myRange.width/100)*sliderPane.width())/segments );
                
                var pos = pxPerSegment*whichSegment + zero;
                
                return rationalizePositionWithRange( pos );
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
                    if ( $scope.range[i] === slider.realRange ){
                        slider.value.range = i;
                        break;
                    }
                }
                
                slider.position = findPositionForValue( slider.value ).position;
                fixStartPositions();
                
                $scope.editing = -1;
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
            
            $scope.sliderEdit = function( $index ){
                $scope.editing = $index;
            };
            
            // initialize the widet.
            var init = function(){
                
                fixRangeWidths();  
                determinePanelWidths();
                
                for ( var i = 0; i < $scope.sliders.length; i++ ){
                    $scope.sliders[i].realRange = $scope.range[ $scope.sliders[i].value.range ];
                }
                
                // needs second for the rendering to complete so that width calculations work properly.  Otherwise we're a variant number
                // of partial pixels off
                $timeout( 
                    function(){
                        // set to the right values.
                        for( var i = 0; i < $scope.sliders.length; i++ ){

                            if ( !angular.isDefined( $scope.sliders[i].value ) ){
                                $scope.sliders[i].value = { range: 0, value: $scope.range[0].start };
                            }

                            $scope.sliders[i].position = findPositionForValue( $scope.sliders[i].value ).position;
                        }
                        
                        fixStartPositions();
                    }, 50 );
            };
            
            $document.on( 'mousemove', null, $scope.sliderMoved );
            $document.on( 'mouseup', null, handleReleased );

            $scope.$on( 'destroy', function(){
                $document.off( 'mousemove', null, $scope.sliderMoved );
                $document.off( 'mouseup', null, handleReleased );
            });
            
            init();
            $resize_service.register( $scope.$id, init );
        }
    };
    
});