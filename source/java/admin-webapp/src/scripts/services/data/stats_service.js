angular.module( 'statistics' ).factory( '$stats_service', ['$http_fds', function( $http_fds ){

    var service = {};

    service.getFirebreakSummary = function( filter, success, failure ){
        
        return $http_fds.put( webPrefix + '/stats/volumes/firebreak', filter, 
            function( response ){
                if ( angular.isFunction( success ) ){
                    success( response );
                }
            },
            function( response, code ){
                if ( angular.isFunction( failure ) ){
                    failure( response, code );
                }
            });
        
//        return $http_fds.get( '/scripts/services/data/fakefirebreak.js', 
//            function( response ){
//                success( eval( response )[0] );
//                
//
//            });
    };
    
    service.getPerformanceBreakdownSummary = function( filter, success, failure ){
        
        $http_fds.put( webPrefix + '/stats/volumes', filter, 
            function( response ){
            
                if ( angular.isFunction( success ) ){
                    success( response );
                }
            },
            function( response, code ){
                if ( angular.isFunction( failure ) ){
                    failure( response, code );
                }
            });
    };
    
    service.getPerformanceSummary = function( filter, success, failure ){
        
//        var data = {
//            series: [
//                {
//                    context: {},
//                    datapoints: [
//                    {x:0, y: 10},
//                    {x:1, y: 40},
//                    {x:2, y: 31},
//                    {x:3, y: 16},
//                    {x:4, y: 19},
//                    {x:5, y: 2},
//                    {x:6, y: 8},
//                    {x:7, y: 52},
//                    {x:8, y: 12},
//                    {x:9, y: 34}]
//                },
//                {
//                    context: {},
//                    datapoints: [
//                    {x:0, y: 15},
//                    {x:1, y: 46},
//                    {x:2, y: 36},
//                    {x:3, y: 21},
//                    {x:4, y: 30},
//                    {x:5, y: 12},
//                    {x:6, y: 10},
//                    {x:7, y: 53},
//                    {x:8, y: 17},
//                    {x:9, y: 39}]
//                }
//            ]
//        };
//        
//        success( data );
        
        return $http_fds.put( webPrefix + '/stats/volumes', filter, 
            function( response ){
                if ( angular.isFunction( success ) ){
                    success( response );
                }
            },
            function( response, code ){
                if ( angular.isFunction( failure ) ){
                    failure( response, code );
                }
            });
        
    };
    
    service.getCapacitySummary = function( filter, success, failure ){
        
//        var data = {
//            series: [
//                {
//                    context:{},
//                    datapoints: [
//                    {x:0, y: 10},
//                    {x:1, y: 18},
//                    {x:2, y: 31},
//                    {x:3, y: 27},
//                    {x:4, y: 41},
//                    {x:5, y: 55},
//                    {x:6, y: 22},
//                    {x:7, y: 31},
//                    {x:8, y: 47},
//                    {x:9, y: 58}]
//                },
//                {
//                    context: {},
//                    datapoints: [{x:0, y: 15},
//                    {x:1, y: 24},
//                    {x:2, y: 36},
//                    {x:3, y: 33},
//                    {x:4, y: 48},
//                    {x:5, y: 60},
//                    {x:6, y: 31},
//                    {x:7, y: 48},
//                    {x:8, y: 54},
//                    {x:9, y: 65}]
//                }
//            ]
//        };
//        
//        success( data );
        return $http_fds.put( webPrefix + '/stats/volumes', filter, 
            function( response ){
                if ( angular.isFunction( success ) ){
                    success( response );
                }
            },
            function( response, code ){
                if ( angular.isFunction( failure ) ){
                    failure( response, code );
                }
            });
    };    

    return service;
}]);
