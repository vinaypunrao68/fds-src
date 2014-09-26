angular.module( 'activity-management' ).controller( 'activityController', ['$scope', '$activity_service', '$authorization', function( $scope, $activity_service, $authorization ){

    $scope.activities = [];

    $scope.activityCallback = function( response ){
        $scope.activities = eval( response );
    };

    $scope.isAllowed = function( permission ){
        return $authorization.isAllowed( permission );
    };

    $activity_service.getActivities( '', '', 1000, $scope.activityCallback );

}]);
