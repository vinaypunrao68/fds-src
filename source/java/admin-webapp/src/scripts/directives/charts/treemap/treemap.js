angular.module( 'charts' ).directive( 'treemap', function(){

    return {
        restrict: 'E',
        replace: true,
        transclude: false,
        templateUrl: 'scripts/directives/charts/treemap/treemap.html',
        scope: { data: '=', max: '=' },
        controller: function( $scope, $element, $resize_service ){

            var root = {};

            var create = function(){

                var map = d3.layout.treemap();
                var nodes = map
                    .size( [$element.width(),$element.height()] )
                    .sticky( true )
                    .children( function( d ){
                        return d.vals;
                    })
                    .value( function( d,i,j ){
                        return d.size;
                    })
                    .nodes( $scope.data );

                root = d3.select( '.chart' );
                var max = $scope.max;

                var colorScale = d3.scale.linear()
                    .domain( [max, max - (max/4 ), max/2, 0] )
                    .range( ['#339933', '#ffff00', '#ff9900', '#ff0000'] )
                    .clamp( true );

                root.selectAll( '.node' ).remove();

                root.selectAll( '.node' ).data( $scope.data.vals ).enter()
                    .append( 'div' )
                    .classed( {'node': true } )
                    .style( {'position': 'absolute'} )
                    .style( {'width': function( d ){ return d.dx; }} )
                    .style( {'height': function( d ){ return d.dy; }} )
                    .style( {'left': function( d ){ return d.x; }} )
                    .style( {'top': function( d ){ return d.y; }} )
                    .style( {'border': '1px solid white'} )
                    .style( {'background-color': function( d ){
                        return colorScale( d.minSinceLastFirebreak );
                    }});

                var refresh = function(){
                    root.selectAll( '.node' ).remove();
                    create();
                };

                $resize_service.register( $scope.id, refresh );

                $scope.$on( '$destory', function(){
                    $resize_service.unregister( $scope.id );
                });
            };

            create();
        }
    };

});
