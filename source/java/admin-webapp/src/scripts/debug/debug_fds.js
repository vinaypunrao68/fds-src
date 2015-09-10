angular.module( 'debug' ).controller( 'debugController', ['$scope', '$http', '$base64', '$interval', function( $scope, $http, $base64, $interval ){
    
    $scope.stats = [];
    
    $scope.listVolColors = [ 'blue' ];
    $scope.listVolLineColor = [ 'black' ];
    $scope.listVolLabels = [ '15 minutes ago', 'Now' ];
    $scope.listVolStats = { metadata: [], series: [{ datapoints:[], type: 'LIST_VOLUME_TIME'}]};
    
    var pollId = -1;
    var myQueue = '';
    
    // interval in seconds
    var interval = 3;
    
    // duration to show in hours
    var duration = 0.25;
    
    var initData = function(){
        
        var points = Math.round( 60*60*duration / interval );
        
        for ( var i = 0; i < points; i++ ){
            $scope.listVolStats.series[0].datapoints.push( {x: (new Date()).getTime() - (1000*60*60*duration - (i*interval*1000)), y: 0} );
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
        };
            
        var declareExchange = function( resopnse ){
            
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
    };
    
    // take the messages and put them in the right stat arrays
    var handleStats = function( response ){
        
        var tempStats = { series: [] };
        tempStats.series = $scope.listVolStats.series;
        var timeSum = 0;
        var numListVols = 0;
        
        for ( var i = 0; i < response.data.length; i++ ){
            var data = response.data[i].payload;
            data = JSON.parse( data );
            
            if ( data.metricName === 'LIST_VOLUME_TIME' ){
                timeSum += data.metricValue;
                numListVols++;
            }
        }
        
        if ( numListVols === 0 ){
            numListVols = 1;
        }
        
        tempStats.series[0].datapoints.push( { x: (new Date()).getTime(), y: timeSum / numListVols } );
        
        // get rid of old points.
        for ( var j = 0; j < tempStats.series[0].datapoints.length; j++ ){
            
            var t = tempStats.series[0].datapoints[j].x;
            
            if ( t < (new Date()).getTime() - 1000*60*60*duration ){
                tempStats.series[0].datapoints.splice( j, 1 );
            }
        }
        
        $scope.listVolStats = tempStats;
    };
    
    $scope.getMessages = function(){
        
        var route = 'api/queues/%2F/' + myQueue + '/get';
        var data = {
            count: 1000,
            requeue: false,
            encoding: 'auto'
        };
        
        makeBusCall( 'POST', route, data ).then( handleStats );
    };
    
    $scope.$on("$destroy", $scope.stop );
    
}]);