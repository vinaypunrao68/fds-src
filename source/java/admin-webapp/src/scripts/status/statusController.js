angular.module( 'status' ).controller( 'statusController', ['$scope', '$activity_service', '$interval', '$authorization', '$stats_service', '$filter', '$interval', function( $scope, $activity_service, $interval, $authorization, $stats_service, $filter, $interval ){
    
    $scope.activities = [];
    $scope.firebreakMax = 1440;
    $scope.firebreakStats = { series: [[]], summaryData: { hoursSinceLastEvent: 0 }};
    $scope.performanceStats = { series: [[]] };
    
    $scope.fakeColors = [ 'green', 'green' ];
    $scope.fakeOpacities = [0.7,0.7];
    
    $scope.activitiesReturned = function( list ){
        $scope.activities = eval(list);
    };
    
    $scope.firebreakReturned = function( data ){
        $scope.firebreakStats = data;
    };
    
    $scope.performanceReturned = function( data ){
        $scope.performanceStats = data;
    };
    
    // this callback creates the tooltip element
    // which will get constructed ultimately by treemap
    $scope.setFirebreakToolipText = function( data ){
        
        var units = '';
        var value = 0;
        
        // set the text as days
        if ( data.secondsSinceLastFirebreak > 24*60*60 ){
            value = Math.floor( data.secondsSinceLastFirebreak / (24*60*60) )
            units = $filter( 'translate' )( 'common.days' );
        }
        // set the text as hours
        else if ( data.secondsSinceLastFirebreak > (60*60) ){
            value = Math.floor( data.secondsSinceLastFirebreak / (60*60) )
            units = $filter( 'translate' )( 'common.hours' );            
        }
        else {
            value = Math.floor( data.secondsSinceLastFirebreak / 60 );
            units = $filter( 'translate' )( 'common.minutes' );
        }
        
        var str = '<div><div style="font-weight: bold;font-size: 11px;">' + data.name + '</div><div style="font-size: 10px;">' + $filter( 'translate' )( 'status.tt_firebreak', { value: value,units: units} ) + '</div></div>';
        
        return str;
    };

    $scope.isAllowed = function( permission ){
        var isit = $authorization.isAllowed( permission );
        return isit;
    };

    // pollers
    var firebreakInterval = $interval( function(){ $stats_service.getFirebreakSummary( $scope.firebreakReturned );}, 10000 );
    
    $activity_service.getActivities( '', '', 30, $scope.activitiesReturned );
    
    // cleanup the pollers
    $scope.$on( '$destroy', function(){
        $interval.cancel( firebreakInterval );
    });
    
    $stats_service.getFirebreakSummary( $scope.firebreakReturned );
    $stats_service.getPerformanceSummary( $scope.performanceReturned );

}]);
