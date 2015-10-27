angular.module( 'debug' ).controller( 'debugController', ['$scope', '$http', '$base64', '$interval', function( $scope, $http, $base64, $interval ){
    
    $scope.stats = [];
    
    $scope.colors = [ 'blue' ];
    $scope.lineColors = [ 'blue' ];
    $scope.timeLabels = [ '15 minutes ago', 'Now' ];
    $scope.statChoices = [{name: 'AM Put Requests', value: 'AM_PUT_OBJ_REQ'}];
    $scope.chart1Data = { metadata: [], series: [{ datapoints:[], type: 'AM_PUT_OBJ_REQ'}]};
    $scope.chart1Choice = undefined;
    
    var pollId = -1;
    var myQueue = '';
    
    // interval in seconds
    var interval = 3;
    
    // duration to show in hours
    $scope.duration = 0.25;
    
    var initData = function(){
        
        var points = Math.round( 60*60*$scope.duration / interval );
        
        for ( var i = 0; i < points; i++ ){
            $scope.chart1Data.series[0].datapoints.push( {x: (new Date()).getTime() - (1000*60*60*$scope.duration - (i*interval*1000)), y: 0} );
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
    };
    
    // take the messages and put them in the right stat arrays
    var handleStats = function( response ){
        $scope.$broadcast( 'fds::new_stats', response.data );
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
    
    $scope.$on("$destroy", $scope.stop );
    
}]);