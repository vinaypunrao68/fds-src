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
        scope: { data: '=ngModel', dateRange: '=', divisions: '@', domain: '=', range: '=', iconClass: '@', rightLabel: '@', leftLabel: '@', middleLabel: '@' },
        controller: function( $scope, $timeout ){
            
            $scope.bucketTime = 0;
            $scope.boxes = [];
            $scope.outOfRangeEvents = [];
            
            $scope.calculateEventPosition = function( event ){
                
                if ( !angular.isDefined( event ) ){
                    return 0;
                }
                
                var bucket = Math.floor( ( parseInt( event.x ) - $scope.dateRange.start ) / $scope.bucketTime );
                var px = bucket * 22;
                
                return px;
            };
            
            var fixColors = function(){
                
                var colorFx = function( value ){
                    var index = 0;
                    
                    for ( var i = 0; i < $scope.domain.length-1; i++ ){
                        
                        var dVal = $scope.domain[i+1];
                        
                        if ( value > dVal ){
                            break;
                        }
                        
                        index++;
                    }
                    
                    return $scope.range[index];
                };
                
                var lastFb = 0;
                
                // this means something happened before the displayed time that may effect our starting point.
                if ( $scope.outOfRangeEvents.length > 0 ){
                    
                    for( var j = 0; j < $scope.outOfRangeEvents.length; j++ ){
                        
                        if ( parseInt( $scope.outOfRangeEvents[j].x ) > lastFb ){
                            lastFb = parseInt( $scope.outOfRangeEvents[j].x );
                        }
                    }
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
                    var bucket = Math.floor( ( parseInt( $scope.data[i].x ) - $scope.dateRange.start)/$scope.bucketTime );
                    
                    if ( !angular.isDefined( bucket ) || !angular.isDefined( $scope.boxes[bucket] ) ){
                        $scope.outOfRangeEvents.push( $scope.data[i] );
                        continue;
                    }
                    
                    $scope.boxes[bucket].y.push( parseInt( $scope.data[i].y ) );
                    $scope.data[i].pos = $scope.calculateEventPosition( $scope.data[i] );
                }
                
                fixColors();
            };
            
            $scope.$watch( 'data', function( newVal, oldVal ){
              
                if ( newVal === oldVal ){
                    return;
                }
                
                init();
            });
            
            $scope.$on( 'fds::refresh-status-timeline', init );
            
            init();
        }
    };
});