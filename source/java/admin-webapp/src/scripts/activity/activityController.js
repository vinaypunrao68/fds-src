angular.module( 'activity-management' ).controller( 'activityController', ['$scope', '$activity_service', '$authorization', function( $scope, $activity_service, $authorization ){

    $scope.activities = [];
    $scope.filter = {};

    $scope.getActivities = function( filter, callback ){
        $activity_service.getActivities( filter, callback );
    };
    
    $scope.activityCallback = function( response ){
        $scope.activities = eval( response );
    };

    $scope.isAllowed = function( permission ){
        return $authorization.isAllowed( permission );
    };

//    $activity_service.getActivities( '', '', 1000, $scope.activityCallback );

}]);
