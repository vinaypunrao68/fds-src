angular.module( 'charts' ).directive( 'treemap', function(){

    return {
        restrict: 'E',
        replace: true,
        transclude: false,
        templateUrl: 'scripts/directives/charts/treemap/treemap.html',
        // the tooltip element here is a call back to let the implementor
        // set the text.
        // colorProerty is the property this class will use to pick a color for the box
        scope: { data: '=', tooltip: '=', colorProperty: '@' },
        controller: function( $scope, $element, $resize_service ){

            var root = {};
            
            var $colorScale;
            $scope.hoverEvent = {};
            $scope.tooltipText = 'None';
            
            var buildColorScale = function(){
                
                var max = d3.max( $scope.data, function( d ){
                    
//                    if ( angular.isDefined( d.secondsSinceLastFirebreak ) ){
                    if ( d.hasOwnProperty( $scope.colorProperty ) ){
                        return d[$scope.colorProperty];
                    }
                    return d.y;
                });
                
                $colorScale = d3.scale.linear()
                    .domain( [max, 3600*12, 3600*6, 3600*3, 3600, 0] )
                    .range( ['#389604', '#68C000', '#C0DF00', '#FCE300', '#FD8D00', '#FF5D00'] )
                    .clamp( true );
            };
            
            var computeMap = function( c ){
                
                // preserve the y value
                for ( var i = 0; i < $scope.data.length; i++ ){
                    
//                    if ( !angular.isDefined( $scope.data[i].secondsSinceLastFirebreak ) ){
                    if ( !$scope.data[i].hasOwnProperty( $scope.colorProperty ) ){
                        $scope.data[i][$scope.colorProperty] = $scope.data[i].y;
                    }
                }
                
                var map = d3.layout.treemap();
                var nodes = map
                    .size( [$element.width(),$element.height()] )
                    .sticky( true )
                    .children( function( d ){
                        return d;
                    })
                    .value( function( d,i,j ){
                        
                        if ( angular.isDefined( d.value ) ){
                            return d.value;
                        }
                        
                        return d.x;
                    })
                    .nodes( $scope.data );
            };

            var create = function(){
                
                buildColorScale();
                computeMap();
                
                root = d3.select( '.chart' );

                root.selectAll( '.node' ).remove();

                root.selectAll( '.node' ).data( $scope.data ).enter()
                    .append( 'div' )
                    .classed( {'node': true } )
                    .attr( 'id', function( d, i ){
                        return 'volume_' + i;
                    })
                    .style( {'position': 'absolute'} )
                    .style( {'width': function( d ){ return d.dx; }} )
                    .style( {'height': function( d ){ return d.dy; }} )
                    .style( {'left': function( d ){ return d.x; }} )
                    .style( {'top': function( d ){ return d.y; }} )
                    .style( {'border': '1px solid white'} )
                    .style( {'background-color': function( d ){
                        if ( d.hasOwnProperty( $scope.colorProperty ) ){
                            return $colorScale( d[ $scope.colorProperty ] );
                        }
                        return $colorScale( 0 );
                    }})
                    .on( 'mouseover', function( d ){
                        if ( angular.isFunction( $scope.tooltip ) ){
                            $scope.tooltipText = $scope.tooltip( d );
                            var el = $( $element[0] ).find( '.tooltip-placeholder' );
                            el.empty();
                            var ttEl = $('<div/>').html( $scope.tooltipText ).contents();
                            el.append( ttEl );
                        }
                    });
            };
            
            $scope.$on( '$destory', function(){
                $resize_service.unregister( $scope.id );
            });
            
            $scope.$watch( 'data', create );
                
            create();
        
            $resize_service.register( $scope.id, create );
        }
    };

});
