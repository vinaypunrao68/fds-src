angular.module( 'statistics' ).factory( '$stats_service', ['$http', function( $http ){

    var service = {};

    service.getFirebreakSummary = function( callback ){

        return $http.get( '/scripts/services/data/fakefirebreak.js' )
            .success( function( response ){
                callback( eval( response )[0] );
            });
    };
    
    service.getPerformanceSummary = function( callback ){
        
        var data = {
            series: [
                [
                    {x:0, y: 10},
                    {x:1, y: 40},
                    {x:2, y: 31},
                    {x:3, y: 16},
                    {x:4, y: 19},
                    {x:5, y: 2},
                    {x:6, y: 8},
                    {x:7, y: 52},
                    {x:8, y: 12},
                    {x:9, y: 34}
                ],
                [
                    {x:0, y: 15},
                    {x:1, y: 46},
                    {x:2, y: 36},
                    {x:3, y: 21},
                    {x:4, y: 30},
                    {x:5, y: 12},
                    {x:6, y: 10},
                    {x:7, y: 53},
                    {x:8, y: 17},
                    {x:9, y: 39}
                ]
            ]
        };
        
        callback( data );
        
    };

    return service;
}]);
