angular.module( 'charts' ).directive( 'stackedLineChart', function(){

    return {
        restrict: 'E',
        replace: true,
        transclude: false,
        templateUrl: 'scripts/directives/charts/stackedlinechart/stackedlinechart.html',
        scope: { data: '=', colors: '=?', opacities: '=?', drawPoints: '@', yAxisLabelFunction: '=?', axisColor: '@', 
            tooltip: '=?', lineColors: '=?', lineStipples: '=?', backgroundColor: '@', domainLabels: '=?' },
        controller: function( $scope, $element, $resize_service, $byte_converter){
            
            $scope.hoverEvent = false;
            
            var root = {};
            
            var $left_label = 0;
            var $left_padding = 0;
            var $right_padding = 0;
            var $bottom_labels = 0;
            var $top_padding = 0;
            var $bottom_padding = 0;
            var chartData = [];
            var $xScale;
            var $yScale;
            var $max;
            var $svg;
            var $xMin = 0;
            var $xMax;
            
            if ( angular.isDefined( $scope.domainLabels ) ){
                $bottom_labels = 20;
            }
            
            if ( $scope.drawPoints !== true && $scope.drawPoints !== false ){
                $scope.drawPoints = false;
            }
            
            if ( !angular.isDefined( $scope.axisColor ) ){
                $scope.axisColor = 'black';
            }
            
            var el = d3.select( $element[0] ).select( '.stacked-line-chart' );
            
            $svg = el.append( 'svg' )
                .attr( 'width', '100%' )
                .attr( 'height', '100%' )
                .on( 'click', function(){
                    var lines = $svg.selectAll('.line')[0];
                
                    var results = [];
                
                    for ( var i = 0; i < lines.length; i++ ){
                        var p = lines[i].getTotalLength();
                        var x = d3.event.layerX;
                        p = lines[i].getPointAtLength( x );
                        
                        var val = $byte_converter.convertBytesToString( $yScale.invert( p.y ) );
                        
                        results.push( val );
                    }
                });
            
            if ( angular.isDefined( $scope.backgroundColor ) ){
                
                $svg.append( 'rect' )
                    .attr( 'width', '100%' )
                    .attr( 'height', $element.height() - ($bottom_labels + $bottom_padding) )
                    .attr( 'class', 'background' )
                    .attr( 'fill', $scope.backgroundColor );
            }
            else {
                $svg.append( 'g' )
                    .attr( 'class', 'background' );
            }
            
            // translate labels
            $scope.getLabelText = function( series ){
                
                if ( angular.isFunction( $scope.tooltip ) ){
                    return $scope.tooltip( series );
                }
                
                return series.type;
            };
            
            // calculate this off of real data.
            var buildMax = function(){
                $max = d3.max( $scope.data.series, function( d ){
                    
                    if ( !angular.isDefined( d.datapoints ) ){
                        return 1;
                    }
                    
                    return d3.max( d.datapoints, function( s ){
                        return s.y + s.y0;
                    });
                });
                
                $max = 1.05*$max;
            };
            
            // calculate off of real data.  Depends on max calculation
            var buildScales = function(){
               
                buildMax();
                
                $xMax = 1;
                
                if ( angular.isDefined( $scope.data.series[0] ) && angular.isDefined( $scope.data.series[0].datapoints ) ){
                    var lastXPos = $scope.data.series[0].datapoints.length - 1;
                    $xMax = $scope.data.series[0].datapoints[lastXPos].x;
                }
                
                $xMin = 0;
                
                if ( angular.isDefined( $scope.data.series[0] ) && angular.isDefined( $scope.data.series[0].datapoints ) ){
                    $xMin = $scope.data.series[0].datapoints[0].x;
                }
                
                $xScale = d3.scale.linear()
                    // all series must have the same number of points
                    .domain( [$xMin,$xMax] )
                    .range( [$left_label + $left_padding, $element.width() - $right_padding] );
                
                $yScale = d3.scale.linear()
                    .domain( [ 0, $max ] )
                    .range( [$element.height() - ($bottom_labels + $bottom_padding), $top_padding] );
            };
            
            var clean = function(){
                $svg.selectAll( '.series-group' ).remove();
            };
            
            var removeGuides = function(){
                
                $svg.selectAll( '.guide-lines' ).remove();
            };
            
            var removeLabels = function(){
                $svg.selectAll( '.x-labels' ).remove();
            };
            
            var drawLabels = function(){

                removeLabels();
                
                // not using the d3 ticks because they 
                // force me to leave weird gaps at the min and max
                // when it's just as easy to do the division.
                var ticks = $scope.domainLabels.length;
                
                // the division is one less than the length because we 
                // only need to add the interval for the middle labels
                var interval = Math.round( ($xMax - $xMin) / (ticks - 1) );
                
                var labelGroup = $svg.append( 'g' )
                    .attr( 'class', 'x-labels' );                    
                
                for ( var i = 0; i < ticks; i++ ){
                    labelGroup.append( 'text' )
                        .attr( 'x', function(){
                            
                            var pos = i*interval + $xMin;
                            var labelWidth = measureText( $scope.domainLabels[i] ).width;
                        
                            if ( (i + 1) == ticks ){
                                pos = $xMax;
                            }
                        
                            pos = $xScale( pos );
                        
                            if ( (i+1) == ticks ){
                                pos -= labelWidth;
                            }
                            else if ( i !== 0 ){
                                pos -= labelWidth/2;
                            }

                            return pos;
                        })
                        // the 4 is just so the tails of y's can be seen
                        .attr( 'y', $element.height() - ($bottom_padding + 5) )
                        .text( $scope.domainLabels[i] )
                        .attr( 'fill', $scope.axisColor )
                        .attr( 'font-size', '11px' );
                }
            };
            
            var update = function(){
                
                removeGuides();
                buildScales();
                
                        // draw labels
                if ( angular.isDefined( $scope.domainLabels ) ){
                    drawLabels();
                }
        
                var area = d3.svg.area()
                    .x( function( d ){
                        return $xScale( d.x );
                    })
                    .y0( function( d ){
                        return $yScale( d.y0 );
                    })
                    .y1( function( d ){
                        return $yScale( d.y + d.y0 );
                    })
                    .interpolate( 'basis' );
                
                $svg.selectAll( '.line' )
//                    .transition()
//                    .duration( 500 )
                    .attr( 'd', function( d ){
                        
                        var vals = []
                    
                        if ( angular.isDefined( d.datapoints ) ){
                            vals = d.datapoints;
                        }
                        vals = area( vals );
                        return vals;
                    });
                
                $svg.selectAll( '.point' )
                    .transition()
                    .duration( 500 )
                    .attr( 'cy', function( d ){
                        return $yScale( d.y );
                    });
                
                var chunks = Math.round( $max / 4 );
                
                chunks = [chunks, chunks*2, chunks*3 ];
                
                var guideGroup = $svg.selectAll( '.guide-lines' ).data( chunks ).enter()
                    .append( 'g' )
                    .attr( 'class', 'guide-lines' );
                
                guideGroup.selectAll( '.guide-lines' ).data( chunks ).enter()
                    .append( 'line' )
                    .attr( 'x1', $xScale( $xMin ) )
                    .attr( 'y1', function( d ){ 
                    
                        if ( isNaN( d ) ){
                            d = 0;
                        }
                    
                        return $yScale( d ); 
                    })
                    .attr( 'x2', function( d ){
                        
                        var val = 0;
                    
                        if ( angular.isDefined( $scope.data.series[0] ) && angular.isDefined( $scope.data.series[0].datapoints ) ){
                            var pos = $scope.data.series[0].datapoints.length-1;
                            val = $xScale( $scope.data.series[0].datapoints[pos].x );
                        }
                        else {
                            val = $xScale( 0 );
                        }
                        
                        return val;
                    })
                    .attr( 'y2', function( d ){ 
                        
                        if ( !angular.isNumber( d ) || isNaN( d ) ){
                            d = 0;
                        }
                    
                        return $yScale( d ); 
                    })
                    .attr( 'stroke', $scope.axisColor )
                    .attr( 'stroke-dasharray', '3,3' );
                
                guideGroup.selectAll( '.guide-lines' ).data( chunks ).enter()
                    .append( 'text' )
                    .text( function( d ){
                    
                        if ( angular.isFunction( $scope.yAxisLabelFunction ) ){
                            return ( $scope.yAxisLabelFunction( d ) );
                        }
                    
                        return d;
                    })
                    .attr( 'x', $xScale( $xMin ) )
                    .attr( 'y', function( d ){
                        
                        if ( isNaN( d ) ){
                            d = 0;
                        }
                        
                        return $yScale( d ) - 4;
                    })
                    .attr( 'fill', $scope.axisColor )
                    .attr( 'font-size', '11px' );
                
                // baseline
                $svg.append( 'line' )
                    .attr( 'class', 'guide-lines' )
                    .attr( 'x1', $xScale( $xMin ) )
                    .attr( 'x2', function( d ){
                    
                        if ( angular.isDefined( $scope.data.series[0] ) && angular.isDefined( $scope.data.series[0].datapoints )){
                            return $xScale( $scope.data.series[0].datapoints.length-1 );
                        }
                    
                        return $xScale( $xMin );
                    })
                    .attr( 'y1', $yScale( 0 ) )
                    .attr( 'y2', $yScale( 0 ) )
                    .attr( 'stroke', 'black' );
                    
            };
            
            var create = function(){
                
                clean();
                buildScales();
                
                var area = d3.svg.area()
                    .x( function( d ){
                        return $xScale( d.x );
                    })
                    .y0( function( d ){
                        return $yScale( d.y0 );
                    })
                    .y1( function( d ){
                        return $yScale( d.y + d.y0 );
                    })
                    .interpolate( 'monotone' );
                
                var stackF = d3.layout.stack()
                    .values( function( d ){
                        
                        if ( !angular.isDefined( d.datapoints ) ){
                            return [];
                        }
                        
                        return d.datapoints;
                    });
                
                var stacks = stackF( $scope.data.series );
//                
//                stacks = stacks.sort( function( a, b ){
//                    
//                    var val = 0;
//                    
//                    while ( angular.isDefined( a.datapoints[val] ) ){
//                        diff = ( b.datapoints[val].y0 - a.datapoints[val].y0 );
//                        
//                        if ( diff != 0 ){
//                            return diff;
//                        }
//                        
//                        val++;
//                    }
//                    
//                    return 0;
//                });
                
                var groups = $svg.selectAll( '.series-group' ).data( stacks ).enter()
                    .insert( 'g', '.guide-lines' )
                    .attr( 'class', 'series-group' );
                
                groups.append( 'path' )
                    .attr( 'stroke', function( d, i ){
                        if ( $scope.lineColors && $scope.colors.length > i ){
                            return $scope.lineColors[i];
                        }
                    
                        return '#ACACAC';
                    })
                    .attr( 'stroke-dasharray', function( d, i ){
                        
                        if ( $scope.lineStipples && $scope.lineStipples.length > i ){
                            return $scope.lineStipples[i];
                        }
                    
                        return null;
                    })
                    .attr( 'fill', function( d, i ){
                        if ( $scope.colors && $scope.colors.length > i ){
                            return $scope.colors[i];
                        }
                        
                        return 'black';
                    })
                    .attr( 'fill-opacity', function( d, i ){
                        if ( $scope.opacities && $scope.opacities.length > i ){
                            return $scope.opacities[i];
                        }
                        
                        return 1.0;
                    })
                    .attr( 'class', 'line' )
                    .attr( 'd', function( d ){
                        
                        var vals = [];
                    
                        if ( angular.isDefined( d.datapoints ) ){
                            vals = d.datapoints;
                        }
                    
                        return area( vals ); 
                    })
                    .on( 'mouseover', function( d, i, j ){
                          if ( angular.isFunction( $scope.tooltip ) ){
                            $scope.tooltipText = $scope.tooltip( d, i, j );
                            var el = $( $element[0] ).find( '.tooltip-placeholder' );
                            el.empty();
                            var ttEl = $('<div/>').html( $scope.tooltipText ).contents();
                            el.append( ttEl );
                        }
                    });
                
                
                groups.selectAll( 'series-group' )
                    .data( function( d, i ){
                        return d;
                    }).enter()
                    .append( 'circle' )
                    .attr( 'class', 'point' )
                    .attr( 'cx', function( d ){
                      return $xScale( d.x );  
                    })
                    .attr( 'cy', function( d ){
                        return 0;
                    })
                    .attr( 'r', 3 )
                    .attr( 'fill', 'white' )
                    .attr( 'stroke', 'black' )
                    .attr( 'style', function( d ){
                        if ( $scope.drawPoints === true ){
                            return 'visiblity: visible';
                        }
                        
                        return 'visibility: hidden';
                    });
                
                update();
            };
            
            $scope.setHoverEvent = function( $event ){
                
                if ( angular.isFunction( $scope.tooltip ) ){
                    $event.target = $event.currentTarget;
                    $scope.hoverEvent = $event;
                }
            };
            
            $resize_service.register( $scope.$id, update );
            
            $scope.$watch( 'data', function( newVal, oldVal ){
                
                if ( !angular.isDefined( newVal ) || newVal.series.length === 0 ){
                    return;
                }
                
                if ( newVal.series.length === oldVal.series.length &&
                   $svg.selectAll( '.series-group' )[0].length !== 0 ){
                    
                    if ( angular.isDefined( oldVal.series[0].datapoints ) && newVal.series[0].datapoints.length === oldVal.series[0].datapoints.length ){
//                        update();
                        create();
                    }
                    else {
                        create();
                    }
                }
                else {
                    create();
                }
            },
            function( newVal, oldVal ) {
                
                return (newVal === oldVal);
            });
        }
    };

});
