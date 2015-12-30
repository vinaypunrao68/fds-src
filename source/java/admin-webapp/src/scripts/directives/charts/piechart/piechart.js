angular.module( 'charts' ).directive( 'pieChart', function(){

    return {
        restrict: 'E',
        replace: true,
        transclude: false,
        templateUrl: 'scripts/directives/charts/piechart/piechart.html',
        scope: { data: '=', pieProperty: '@', colors: '=?', opacities: '=?', tooltip: '=?', backgroundColor: '@' },
        controller: function( $scope, $element, $resize_service ){
            
            $scope.hoverEvent = false;
            
            var $svg = undefined;
            var $wedges = undefined;
            var $paths = undefined;
            var radius = 50;
            var padding = 10;
            var lastPie = [];
            
            var GROWTH_POTENTIAL = 0.2;
            
            if ( !angular.isDefined( $scope.colors ) ){
                $scope.colors = ['green', 'red', 'blue', 'yellow', 'orange', 'cyan', 'purple', 'gray'];
            }
            
            // arc drawing functions
            var arc = function(){ return 0; };
            var megaArc = function(){ return 0; };
            
            var setArcFunctions = function(){
                arc = d3.svg.arc().innerRadius( 0 ).outerRadius( radius );
                megaArc = d3.svg.arc().innerRadius( 0 ).outerRadius( radius * (1.0+GROWTH_POTENTIAL) );
            };
            
            var determineRadius = function(){
                
                // take the smaller side
                var full_side = $element.width();
                
                if ( $element.height() < full_side ){
                    full_side = $element.height();
                }
                
                var side = full_side * (1.0-GROWTH_POTENTIAL);
                radius = side / 2;
                padding = (full_side - side)/2;
                
                // re-set them
                setArcFunctions();
            };
            
            var create = function(){
                
                determineRadius();
                var el = d3.select( $element[0] ).select( '.pie-chart' );
            
                $svg = el.append( 'svg' )
                    .data([$scope.data])
                    .attr( 'width', '100%' )
                    .attr( 'height', '100%' );
                
                $wedges = $svg.append( 'g' )
                    .attr( 'class', 'wedge-group' )
                    .attr( 'transform', 'translate( ' + (radius + padding) + ',' + (radius + padding) + ')' );
                
                if ( !angular.isDefined( $scope.pieProperty ) ){
                    $scope.pieProperty = 'value';
                }
            };
            
             
            // construct a pie specific layout
            var pied = d3.layout.pie()
                .value( function( d ){
                    
                    if ( d.hasOwnProperty( $scope.pieProperty ) ){
                        return d[$scope.pieProperty];
                    }
                    
                    return 0;
                })
                .sort( null );
            
            var wedgeSelected = function( d, i, j ){
                
                d3.select( this ).transition()
                    .duration( 200 )
                    .attr( 'd', megaArc );
                
                //handle the tooltip
                if ( angular.isFunction( $scope.tooltip ) ){
                    $scope.tooltipText = $scope.tooltip( d.data, i, j );
                    var el = $( $element[0] ).find( '.tooltip-placeholder' );
                    el.empty();
                    var ttEl = $('<div/>').html( $scope.tooltipText ).contents();
                    el.append( ttEl );
                }
            };
            
            var wedgeDeselected = function(){
                
                d3.select( this ).transition()
                    .duration( 200 )
                    .attr( 'd', arc );
            };
            
            $scope.setHoverEvent = function( $event ){
                
                if ( angular.isFunction( $scope.tooltip ) ){
                    $event.target = $event.currentTarget;
                    $scope.hoverEvent = $event;
                }
            };
            
            // the function that governs how we animate between values;
            var arcTween = function(finish, iter ) {
                var start = {
                    startAngle: 0,
                    endAngle: 0
                };
                
                // we really want to animate from the last known angles - not 0
                if ( angular.isDefined( lastPie[iter].startAngle ) ){
                    start.startAngle = lastPie[iter].startAngle;
                    start.endAngle = lastPie[iter].endAngle;
                }
                
                lastPie[iter] = finish;
                
                var i = d3.interpolate(start, finish);
            
                return function(d) { return arc(i(d)); };
            };
            
            // this will construct the initial arcs.  Anytime we have new number
            // of things this will clean the old chart and draw a new one.
            var createPaths = function(){
                
                // clean
                $wedges.selectAll( '.wedge' ).remove();
                $wedges.selectAll( '.empty' ).remove();
                
                $paths = $wedges.selectAll( '.wedge' ).data( pied($scope.data) ).enter()
                    .append( 'path' )
                    .attr( 'class', 'wedge' )
                    .attr( 'd', arc )
                    .attr( 'fill', 'black' )
                    .attr( 'stroke', 'white' )
                    .attr( 'stroke-width', 2 )
                    .on( 'mouseover', wedgeSelected )
                    .on( 'mouseleave', wedgeDeselected );
            };
            
            // called when the data changes.
            var update = function(){
                
                if ( !angular.isDefined( $paths ) ){
                    return;
                }
                
                pied( $scope.data );
                
                $wedges.selectAll( '.wedge' )
                    .data( pied($scope.data) )
                    .attr( 'fill', function( d, i ){
                    
                        if ( angular.isFunction( $scope.colors ) ){
                            return $scope.colors( d.data, i );
                        }
                        else {
                            return $scope.colors[ i % $scope.colors.length ];
                        }
                    })
                    .transition()
                    .duration( 500 )
                    .attrTween( 'd', arcTween );
            };
            
            // notice a resize and update the chart in case it can grow
            $resize_service.register( $scope.$id, update );
            
            $scope.$watch( 'data', function( newVal, oldVal ){
                
                if ( !angular.isDefined( newVal ) && newVal !== oldVal ){
                    return;
                }
                
                var pathNumber = $svg.selectAll( 'path' )[0].length;
                
                if ( pathNumber < newVal.length ){
                    
                    var laidOut = pied( newVal );
                    
                    // checking to make sure we're not zeroed out
                    for ( var lo = 0; lo < laidOut.length; lo ++ ){
                        if ( isNaN( laidOut[lo].startAngle ) || isNaN( laidOut[lo].endAngle ) ){
                            // yup, we're all zeros or otherwise bad data.  draw the gray donut.
                            $wedges.append( 'path' )
                                .attr( 'fill', '#DDDDDD' )
                                .attr( 'class', 'empty' )
                                .attr( 'd', d3.svg.arc().innerRadius( 0 ).outerRadius( radius ).startAngle( 0 ).endAngle( Math.PI * 2 ) );
                            
                            return;
                        }
                    }
                    
                    createPaths();
                    
                    lastPie = [];
                    
                    // setting up remembering what to animate from
                    for ( var i = 0; i < newVal.length; i++ ){
                        lastPie.push( { startAngle: 0, endAngle: 0 } );
                    }
                }
                
                update();
            },
            function( newVal, oldVal ){
                return ( newVal === oldVal );
            });
            
            create();

        }
    };
});