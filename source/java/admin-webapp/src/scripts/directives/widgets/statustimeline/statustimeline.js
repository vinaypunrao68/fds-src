angular.module( 'display-widgets' ).directive( 'statusTimeline', function(){
    
    return {
        replace: true,
        restrict: 'E',
        transclude: false,
        templateUrl: 'scripts/directives/widgets/statustimeline/statustimeline.html',
        // colorScale [{ value: , color: }, ... ]
        // data: [{ x: time, y: obj }
        // dateRange: { start: , end: }
        // lastEvent:  # in seconds
        scope: { data: '=ngModel', dateRange: '=', divisions: '@', domain: '=', range: '=', lastEvent: '=?', iconClass: '@', rightLabel: '@', leftLabel: '@', middleLabel: '@' },
        controller: function( $scope ){
            
            $scope.bucketTime = 0;
            $scope.boxes = [];
            
            var fixColors = function(){
                
                var colorFx = d3.scale.linear()
                    .domain( $scope.domain )
                    .range( $scope.range )
                    .clamp( true );
                
                var lastFb = 0;
                
                if ( angular.isDefined( $scope.lastEvent ) ){
                    lastFb = $scope.lastEvent;
                }
                
                for ( var i = 0; i < $scope.boxes.length; i++ ){
                    
                    var box = $scope.boxes[i];
                    
                    var diff = box.x - lastFb;
                    
                    if ( box.y.length === 0 ){
                        box.color = colorFx( diff );
                    }
                    else {
                        lastFb = box.x;
                        
                        box.color = $scope.range[ $scope.range.length-1 ];
                    }
                }
            };
            
            var init = function(){
                
                $scope.boxes = [];
                
                var time = $scope.dateRange.end - $scope.dateRange.start;
                $scope.bucketTime = time / parseInt( $scope.divisions );
                
                for ( var b = 0; b < parseInt( $scope.divisions ); b++ ){
                    $scope.boxes.push( { x: $scope.dateRange.start + (b*$scope.bucketTime), y: [] } );
                }
                
                for ( var i = 0; i < $scope.data.length; i++ ){
                    var bucket = ($scope.data[i].x - $scope.dateRange.start)/$scope.bucketTime;
                    
                    $scope.boxes[bucket].y.push( $scope.data[i].y );
                }
                
                fixColors();
            };
            
            $scope.$on( 'fds::refresh-status-timeline', init );
            
            init();
        }
    };
});