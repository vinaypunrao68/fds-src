angular.module( 'debug' ).controller( 'debugController', ['$scope', '$http', '$base64', function( $scope, $http, $base64 ){
    
    $scope.stats = [];
    
    $scope.getStat = function(){
        
        var authStr = $base64.encode( 'admin:admin' );
        
        var req = {
            url: 'http://localhost:15672/api/queues',
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