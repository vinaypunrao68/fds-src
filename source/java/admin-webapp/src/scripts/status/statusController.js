angular.module( 'status' ).controller( 'statusController', ['$scope', '$activity_service', '$interval', '$authorization', '$stats_service', '$filter', '$interval', function( $scope, $activity_service, $interval, $authorization, $stats_service, $filter, $interval ){
    
    $scope.activities = [];
    $scope.firebreakMax = 1440;
    $scope.firebreakStats = { series: [[]], summaryData: { hoursSinceLastEvent: 0 }};
    $scope.performanceStats = { series: [[]] };
    $scope.capacityStats = { series: [[]] };
    
    $scope.fakeColors = [ '#73DE8C', '#73DE8C' ];
    $scope.fakeCapColors = [ '#71AEEA', '#AAD2F4' ];
    $scope.fakeOpacities = [0.7,0.7];
    
    $scope.capacityLineStipples = ['none', '2,2'];
    $scope.capacityLineColors = ['#1C82FB', '#71AFF8'];
    
    $scope.activitiesReturned = function( list ){
        $scope.activities = eval(list);
    };
    
    $scope.firebreakReturned = function( data ){
        $scope.firebreakStats = data;
    };
    
    $scope.performanceReturned = function( data ){
        $scope.performanceStats = data;
    };
    
    $scope.capacityReturned = function( data ){
        $scope.capacityStats = data;
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
        
        var str = '<div><div style="font-weight: bold;font-size: 11px;">' + data.context.name + '</div><div style="font-size: 10px;">' + $filter( 'translate' )( 'status.tt_firebreak', { value: value,units: units} ) + '</div></div>';
        
        return str;
    };
    
    $scope.setCapacityTooltipText = function( data, i, j ){
        if ( i == 0 ){
            return $filter( 'translate' )( 'status.desc_dedup_capacity' );
        }
        else {
            return $filter( 'translate' )( 'status.desc_pre_dedup_capacity' );
        }
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
    $stats_service.getCapacitySummary( $scope.capacityReturned );

}]);
