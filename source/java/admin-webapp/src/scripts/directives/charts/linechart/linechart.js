angular.module( 'charts' ).directive( 'lineChart', function(){

    return {
        restrict: 'E',
        replace: true,
        transclude: false,
        templateUrl: 'scripts/directives/charts/linechart/linechart.html',
        scope: { data: '=', colors: '=?', opacities: '=?', drawPoints: '@' },
        controller: function( $scope, $element, $resize_service ){
            
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
            
            if ( $scope.drawPoints !== true && $scope.drawPoints !== false ){
                $scope.drawPoints = false;
            }
            
            $svg = d3.select( '.line-chart' )
                .append( 'svg' )
                .attr( 'width', '100%' )
                .attr( 'height', '100%' );
            
            // calculate this off of real data.
            var buildMax = function(){
                $max = d3.max( $scope.data.series, function( d ){
                    return d3.max( d, function( s ){
                        return s.y;
                    });
                });
            };
            
            // calculate off of real data.  Depends on max calculation
            var buildScales = function(){
                
                buildMax();
                
                $xScale = d3.scale.linear()
                    // all series must have the same number of points
                    .domain( [0,$scope.data.series[0].length] )
                    .range( [$left_label + $left_padding, $element.width() - $right_padding] );
                
                $yScale = d3.scale.linear()
                    .domain( [ 0, $max ] )
                    .range( [$element.height() - $top_padding, $bottom_labels + $bottom_padding] );
            };
            
            var clean = function(){
                $svg.removeAll();
            };
            
            var update = function(){
                
                buildScales();
                
                var area = d3.svg.area()
                    .x( function( d ){
                        return $xScale( d.x );
                    })
                    .y0( function( d ){
                        return $yScale( 0 );
                    })
                    .y1( function( d ){
                        return $yScale( d.y );
                    })
                    .interpolate( 'monotone' );
                
                $svg.selectAll( '.line' )
                    .transition()
                    .duration( 500 )
                    .attr( 'd', area );
                
                $svg.selectAll( '.point' )
                    .transition()
                    .duration( 500 )
                    .attr( 'cy', function( d ){
                        return $yScale( d.y );
                    });
            };
            
            var create = function(){
                
                buildScales();
                
                var area = d3.svg.area()
                    .x( function( d ){
                        return $xScale( d.x );
                    })
                    .y0( function( d ){
                        return $yScale( 0 );
                    })
                    .y1( function( d ){
                        return $yScale( 0 );
                    })
                    .interpolate( 'monotone' );
                
                var groups = $svg.selectAll( '.series-group' ).data( $scope.data.series ).enter()
                    .append( 'g' )
                    .attr( 'class', 'series-group' );
                
                                groups.append( 'path' )
                    .attr( 'stroke', '#ACACAC' )
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
                        
                        return 0.8;
                    })
                    .attr( 'class', 'line' )
                    .attr( 'd', area );
                
                
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

            create();
            
            $scope.$watch( 'data', function( oldVal, newVal ){
                
            });
        }
    };

});
