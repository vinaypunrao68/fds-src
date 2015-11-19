angular.module( 'charts' ).directive( 'statsLineChart', function(){

    return {
        restrict: 'E',
        replace: true,
        transclude: false,
        templateUrl: 'scripts/directives/fds-components/stats-line-chart/stats-line-chart.html',
        // duration is in hours, interval is in seconds
        scope: { data: '=ngModel', colors: '=?', lineColors: '=?', yLabelFunction: '=?', domainLabels: '=?', stat: '=', lineType: '@', duration: '=?', title:"=?", min: "=?", max: "=?" },
        controller: function( $scope, $element ){
            
            if ( !angular.isDefined( $scope.duration ) ){
                $scope.duration = 1;
            }
            
            if ( !angular.isDefined( $scope.interval ) ){
                $scope.interval = 3;
            }
            
            if ( !angular.isDefined( $scope.title ) ){
                title = $scope.stat;
            }
            
            // new stats are reported through an event so you can have 1 master poller.
            $scope.$on( 'fds::new_stats', function( event, newStats ){
                
                var tempStats = { series: [] };
                tempStats.series = $scope.data.series;
                
                var timeSum = 0;
                var numStats = 0;

                for ( var i = 0; i < newStats.length; i++ ){
                    
                    var data = newStats[i].payload;
                    data = JSON.parse( data );
                    
                    if ( data.metricName === $scope.stat ){
//                        console.log( 'Found one: ' + data.metricName + ': ' + parseInt(data.metricValue ) );
                        timeSum += parseInt( data.metricValue );
                        numStats++;
                    }
                }

                if ( numStats === 0 ){
                    numStats = 1;
                }

                tempStats.series[0].datapoints.push( { x: (new Date()).getTime(), y: timeSum / numStats } );

                // get rid of old points.
                for ( var j = 0; j < tempStats.series[0].datapoints.length; j++ ){

                    var t = tempStats.series[0].datapoints[j].x;

                    if ( t < (new Date()).getTime() - 1000*60*60*$scope.duration ){
                        tempStats.series[0].datapoints.splice( j, 1 );
                    }
                }

                $scope.data = tempStats;
                
            });
            
        }
    };
});