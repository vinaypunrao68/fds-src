angular.module( 'debug' ).controller( 'debugController', ['$scope', '$http', '$base64', function( $scope, $http, $base64 ){
    
    $scope.stats = [];
    
    $scope.listVolColors = [ 'blue' ];
    $scope.listVolLineColor = [ 'black' ];
    $scope.listVolLabels = [ '2 hours ago', 'Now' ];
    $scope.listVolStats = [];
    
    var pollId = -1;
    var myQueue = '';
    
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
                        
                        pollId = $interval( $scope.getMessages, 2 );
    
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
        $interval.cancel( pollId );
        makeBusCall( 'DELETE', 'api/queueus/%2F/' + myQueue );
    };
    
    // take the messages and put them in the right stat arrays
    var handleStats = function( response ){
        
        for ( var i = 0; i < response.data.length; i++ ){
            alert( 'Msg: ' + i );
        }
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