angular.module( 'charts' ).directive( 'treemap', function(){

    return {
        restrict: 'E',
        replace: true,
        transclude: false,
        templateUrl: 'scripts/directives/charts/treemap/treemap.html',
        scope: { data: '=' },
        controller: function( $scope, $element, $resize_service ){

            var root = {};
            
            var $colorScale;
            
            var buildColorScale = function(){
                
                var max = d3.max( $scope.data, function( d ){
                    
                    if ( angular.isDefined( d.secondsSinceLastFirebreak ) ){
                        return d.secondsSinceLastFirebreak;
                    }
                    return d.y;
                });
                
                $colorScale = d3.scale.linear()
                    .domain( [max, max - (max/4 ), max/2, 0] )
                    .range( ['#339933', '#ffff00', '#ff9900', '#ff0000'] )
                    .clamp( true );
            };
            
            var computeMap = function( c ){
                
                // preserve the y value
                for ( var i = 0; i < $scope.data.length; i++ ){
                    
                    if ( !angular.isDefined( $scope.data[i].secondsSinceLastFirebreak ) ){
                        $scope.data[i].secondsSinceLastFirebreak = $scope.data[i].y;
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
                    .style( {'position': 'absolute'} )
                    .style( {'width': function( d ){ return d.dx; }} )
                    .style( {'height': function( d ){ return d.dy; }} )
                    .style( {'left': function( d ){ return d.x; }} )
                    .style( {'top': function( d ){ return d.y; }} )
                    .style( {'border': '1px solid white'} )
                    .style( {'background-color': function( d ){
                        return $colorScale( d.secondsSinceLastFirebreak );
                    }});
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
