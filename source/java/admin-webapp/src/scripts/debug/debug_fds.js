angular.module( 'debug' ).controller( 'debugController', ['$scope', '$http', '$base64', '$interval', '$stats_push_helper', function( $scope, $http, $base64, $interval, $stats_push_helper ){
    
    $scope.stats = [];
    
    $scope.durations = [{name: '1hr', value: 1, timeLabels: ['1 hour ago', 'Now']},
                        {name: '30min', value: 0.5, timeLabels: ['30 minutes ago', 'Now']},
                        {name: '15min', value: 0.25, timeLabels: ['15 minutes ago', 'Now']},
                        {name: '5min', value: (1/12), timeLabels: ['5 minutes ago', 'Now']},
                        {name: '1min', value: (1/60), timeLabels: ['1 minute ago', 'Now']}];
    
    $scope.durationChoice = $scope.durations[2];
    $scope.timeLabels = $scope.durationChoice.timeLabels;
    
    $scope.isEnabled = true;
    $scope.colors = [ 'blue' ];
    $scope.lineColors = [ 'blue' ];
    $scope.statChoices = [{name: 'None'},
                          {name: 'AM PUT Requests', value: 'AM_PUT_OBJ_REQ'},
                          {name: 'AM GET Requests', value: 'AM_GET_OBJ_REQ'},
                          {name: 'DM Transaction Latency', value: 'DM_TX_OP'},
                          {name: 'Latency of PUT in SM', value: 'SM_PUT_IO'}];
    
    $scope.chartData = [ { metadata: [], series: [{ datapoints:[], type: 'AM_PUT_OBJ_REQ'}]},
                         { metadata: [], series: [{ datapoints:[], type: 'AM_PUT_OBJ_REQ'}]},
                         { metadata: [], series: [{ datapoints:[], type: 'AM_PUT_OBJ_REQ'}]},
                         { metadata: [], series: [{ datapoints:[], type: 'AM_PUT_OBJ_REQ'}]}];
    
    $scope.min = undefined;
    $scope.max = undefined;
    $scope.syncRange = false;
    
    $scope.chartType = ['sum', 'sum', 'sum', 'sum'];
    $scope.chartChoice = [$scope.statChoices[0], $scope.statChoices[0], $scope.statChoices[0], $scope.statChoices[0]];
    $scope.rangeMaximum = undefined;
    $scope.rangeMinimum = undefined;
    
    var pollId = -1;
    var myQueue = '';
    
    // interval in seconds
    var interval = 3;
    
    // duration to show in hours
    $scope.duration = $scope.durationChoice.value;
    
    var initData = function(){
        
        var points = Math.round( 60*60*$scope.duration / interval );
        
        for ( var i = 0; i < points; i++ ){
            for( var j = 0; j < $scope.chartData.length; j++ ){
                $scope.chartData[j].series[0].datapoints.push( {x: (new Date()).getTime() - (1000*60*60*$scope.duration - (i*interval*1000)), y: 0} );
            }
        }
    };
    
    var makeBusCall = function( method, route, data ){
        
        var realRoute = encodeURIComponent( route );
        
        var req = {
            url: '/fds/config/v08/mb/' + realRoute,
            headers: {
                'Content-Type': 'application/json'
            },
            method: method,
            data: data
        };
        
        return $http( req );
    };
    
    $scope.amPutLabel = function( value ){
        
        return value + ' ms';
    };
    
    $scope.start = function(){
        
        if ( myQueue !== '' ){
            return;
        }
        
        myQueue = 'ui-queue-' + (new Date()).getTime();
        
        // create my queue
        var route = 'api/queues/%2F/' + myQueue;
        var data = {
            'auto_delete': true,
            'durable': false,
            'arguments': {},
            'node': "rabbit"
        };
        
        // setup a binding
        var bindToExchange = function( response ){
            
            var route = 'api/bindings/%2F/e/admin/q/' + myQueue;
            var data = {
                'routing_key': 'stats.VOLUME.#',
                'arguments': []
            };
            
            makeBusCall( 'POST', route, data )
                .then( 
                    function( response ){ 
                        alert( 'Successfully bound.' );
                        
                        initData();
                        pollId = $interval( $scope.getMessages, interval*1000 );
    
                    },
                    function( response ){
                        alert( 'Failed to bind to a subscription' );
                    });
            
            $scope.isEnabled = false;
        };
            
        var declareExchange = function( response ){
            
            var route = 'api/exchanges/%2F/admin';
            var data = {
                'type': 'topic',
                'auto_delete': false,
                'durable': false,
                'arguments': []
            };
            
            makeBusCall( 'PUT', route, data )
                .then( bindToExchange,
                      function( response ){
                        alert( 'Failed to declare exchange' );
                    });
        };
        
        makeBusCall( 'PUT', route, data )
            .then( declareExchange, 
                  function( response )
                  { 
                    alert( 'Failed to establish queue' );
                  });
        
    };
    
    // stop the poller for stats.
    $scope.stop = function(){
        
        if ( pollId === -1 ){
            return;
        }
        
        $interval.cancel( pollId );
        
        var data = {
            mode: "delete",
            name: myQueue,
            vhost: "/"
        };
        
        makeBusCall( 'DELETE', 'api/queues/%2F/' + myQueue, data );
        
        myQueue = '';
        $scope.isEnabled = true;
    };
    
    // take the messages and put them in the right stat arrays
    var handleStats = function( response ){
        
        if ( $scope.syncRange === true ){
            
            var min = undefined;
            var max = 0;
            
            // go through each data set
            for ( var ii = 0; ii < $scope.chartData.length; ii++ ){
                
                var chartData = $scope.chartData[ii];
                
                // go through each series
                for ( var jj = 0; jj < chartData.series.length; jj++ ){
                    
                    var series = chartData.series[jj];
                    
                    // go through each datapoint to find a min and max.
                    for ( var kk = 0; kk < series.datapoints.length; kk++ ){
                        
                        var datapoint = series.datapoints[kk];
                        
                        if ( !angular.isDefined( min ) || datapoint.y < min ){
                            min = datapoint.y;
                        }
                        
                        if ( datapoint.x > max ){
                            max = datapoint.x;
                        }
                        
                    }// kk
                }// jj
            }// ii 
            
            $scope.rangeMinimum = min;
            $scope.rangeMaximum = max;
        }
        
        var cStats = $stats_push_helper.convertBusDataToPoints( response.data );
        
        for ( var dataIt = 0; dataIt < $scope.chartData.length; dataIt++ ){
            
            $stats_push_helper.push_stat( $scope.chartData[dataIt], 
                                          cStats, 
                                          [$scope.chartChoice[dataIt].value], 
                                          $scope.durationChoice.value * 1000*60*60, 
                                          ($scope.chartType[dataIt] === 'avg' ) );
        }
    };
    
    $scope.getMessages = function(){
        
        var route = 'api/queues/%2F/' + myQueue + '/get';
        var data = {
            count: 1000,
            requeue: false,
            encoding: 'auto'
        };
        
        makeBusCall( 'POST', route, data ).then( handleStats, function(){} );
    };
    
    $scope.$watch( 'durationChoice', function( newVal ){
        $scope.duration = newVal.value;
        $scope.timeLabels = newVal.timeLabels;
    });
    
    $scope.$on("$destroy", $scope.stop );
    
}]);