angular.module( 'status' ).controller( 'statusController', ['$scope', '$activity_service', '$interval', '$authorization', '$stats_service', function( $scope, $activity_service, $interval, $authorization, $stats_service ){

//    $scope.fakeData = {
//        series: [
//          [
//              {x:0, y: 10},
//              {x:1, y: 40},
//              {x:2, y: 31},
//              {x:3, y: 16},
//              {x:4, y: 19},
//              {x:5, y: 2},
//              {x:6, y: 8},
//              {x:7, y: 52},
//              {x:8, y: 12},
//              {x:9, y: 34}
//          ],
//          [
//              {x:0, y: 15},
//              {x:1, y: 46},
//              {x:2, y: 36},
//              {x:3, y: 21},
//              {x:4, y: 30},
//              {x:5, y: 12},
//              {x:6, y: 10},
//              {x:7, y: 53},
//              {x:8, y: 17},
//              {x:9, y: 39}
//          ]
//        ]
//    };
//    
//    $scope.fakeColors = [ 'red', 'red' ];
//    $scope.fakeOpacities = [0.5, 0.5];
    
    $scope.activities = [];
    $scope.firebreakMax = 1440;
    $scope.firebreakStats = { series: [[]], summaryData: { hoursSinceLastEvent: 0 }};
    
    $scope.activitiesReturned = function( list ){
        $scope.activities = eval(list);
    };
    
    $scope.firebreakReturned = function( data ){
        $scope.firebreakStats = data;
    };

    $scope.isAllowed = function( permission ){
        var isit = $authorization.isAllowed( permission );
        return isit;
    };

    $activity_service.getActivities( '', '', 30, $scope.activitiesReturned );
    $stats_service.getFirebreakSummary( $scope.firebreakReturned );

}]);
