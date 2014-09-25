angular.module( 'status' ).controller( 'statusController', ['$scope', '$activity_service', '$interval', '$authorization', '$stats_service', function( $scope, $activity_service, $interval, $authorization, $stats_service ){

    $scope.activities = [];
    $scope.firebreakStats = $stats_service.getFirebreakSummary();
    $scope.firebreakMax = 1440;

    $scope.activitiesReturned = function( list ){
        $scope.activities = eval(list);
    };

    $scope.isAllowed = function( permission ){
        var isit = $authorization.isAllowed( permission );
        return isit;
    };

    $activity_service.getActivities( '', '', 30, $scope.activitiesReturned );

}]);
