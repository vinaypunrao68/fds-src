angular.module( 'debug' ).controller( 'debugController', ['$scope', '$http', '$base64', function( $scope, $http, $base64 ){
    
    $scope.stats = [];
    
    $scope.getStat = function(){
        
        var authStr = $base64.encode( 'admin:admin' );
        var route = '/api/queues';
        route = encodeURI( route );
        
        var req = {
            url: '/fds/config/v08/mb/' + route,
            headers: {
                Authorization: 'Basic ' + authStr
            },
            method: 'GET'
        };
        
        $http( req )
            .then( function( response ){
            
                alert( response );
            });
        
    };
    
}]);