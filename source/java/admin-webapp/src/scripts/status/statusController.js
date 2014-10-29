angular.module( 'status' ).controller( 'statusController', ['$scope', '$activity_service', '$interval', '$authorization', '$stats_service', '$filter', '$interval', '$byte_converter', function( $scope, $activity_service, $interval, $authorization, $stats_service, $filter, $interval, $byte_converter ){
    
    $scope.items = [
        {number: 12.5, description: 'This is a number and a really long line of text that we hope wraps'},
        {number: 79, description: 'Something else', suffix: '%' }
    ];
    
    $scope.activities = [];
    $scope.firebreakMax = 1440;
    $scope.firebreakStats = { series: [[]], summaryData: { hoursSinceLastEvent: 0 }};
    $scope.performanceStats = { series: [[]] };
    $scope.capacityStats = { series: [[]] };
    
    $scope.firebreakDomain = [ 'max', 3600*12, 3600*6, 3600*3, 3600, 0 ];
    $scope.firebreakRange = ['#389604', '#68C000', '#C0DF00', '#FCE300', '#FD8D00', '#FF5D00'];
    
    $scope.fakeColors = [ '#73DE8C', '#73DE8C' ];
    $scope.fakeCapColors = [ '#71AEEA', '#AAD2F4' ];
    $scope.fakeOpacities = [0.7,0.7];
    
    $scope.capacityLineStipples = ['none', '2,2'];
    $scope.capacityLineColors = ['#1C82FB', '#71AFF8'];
    
    $scope.capacityLabels = [ $filter( 'translate' )( 'common.l_30_days' ), $filter( 'translate' )( 'common.l_15_days' ), $filter( 'translate' )( 'common.l_today' )];
    $scope.performanceLabels = [ $filter( 'translate' )( 'common.l_1_hour' ), $filter( 'translate' )( 'common.l_now' )];
    
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
    
    $scope.capacityLabelFx = function( data ){
        return $byte_converter.convertBytesToString( data, 0 );
    };

    $scope.transformFirebreakTime = function( value ){
        return value;
    };
    
    $scope.isAllowed = function( permission ){
        var isit = $authorization.isAllowed( permission );
        return isit;
    };

    // pollers
    var buildFirebreakFilter = function(){
        var filter = StatQueryFilter.create( [], 
            [ StatQueryFilter.SHORT_TERM_CAPACITY_SIGMA,
             StatQueryFilter.LONG_TERM_CAPACITY_SIGMA,
             StatQueryFilter.SHORT_TERM_PERFORMANCE_SIGMA,
             StatQueryFilter.LONG_TERM_PERFORMANCE_SIGMA],
            (new Date()).getTime() - (1000*60*60*24),
            (new Date()).getTime() );
    
        return filter;
    };
                                                            
    var firebreakInterval = $interval( function(){ $stats_service.getFirebreakSummary( buildFirebreakFilter(), $scope.firebreakReturned );}, 60000 );
    
    $activity_service.getActivities( '', '', 30, $scope.activitiesReturned );
    
    // cleanup the pollers
    $scope.$on( '$destroy', function(){
        $interval.cancel( firebreakInterval );
    });
    
    $stats_service.getFirebreakSummary( buildFirebreakFilter(), $scope.firebreakReturned );
    $stats_service.getPerformanceSummary( {}, $scope.performanceReturned );
    $stats_service.getCapacitySummary( {}, $scope.capacityReturned );

}]);
