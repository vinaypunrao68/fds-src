angular.module( 'activity-management' ).controller( 'activityController', ['$scope', '$activity_service', '$authorization', '$time_converter', function( $scope, $activity_service, $authorization, $time_converter ){

    $scope.activities = [];
    $scope.filter = {points: 50, orderBy: { fieldName: 'initialTimestamp', ascending: false }};

    $scope.getActivities = function( filter, callback ){
        $activity_service.getActivities( filter, callback );
    };
    
    $scope.activityCallback = function( response ){
        $scope.activities = response.events;
    };
    
        
    $scope.getActivityClass = function( category ){
        return $activity_service.getClass( category );
    };
    
    $scope.getActivityCategoryString = function( category ){
        return $activity_service.getCategoryString( category );
    };
    
    $scope.getTimeAgo = function( time ){
        return $time_converter.convertToTimePastLabel( time );
    };

    $scope.isAllowed = function( permission ){
        return $authorization.isAllowed( permission );
    };

    $activity_service.getActivities( $scope.filter, $scope.activityCallback );

}]);
