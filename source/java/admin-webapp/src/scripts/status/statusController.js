angular.module( 'status' ).controller( 'statusController', ['$scope', '$activity_service', '$interval', '$authorization', '$authentication', '$stats_service', '$filter', '$interval', '$byte_converter', '$time_converter', '$rootScope', function( $scope, $activity_service, $interval, $authorization, $authentication, $stats_service, $filter, $interval, $byte_converter, $time_converter, $rootScope ){

    $scope.healthStatus = [{number: 'Excellent'}];
    
    var firebreakInterval = -1;
    var performanceInterval = -1;
    var capacityInterval = -1;
    var activityInterval = -1;
    var perfBreakdownInterval = -1;
    
    
    $scope.activities = [];
    $scope.firebreakMax = 1440;
    $scope.minArea = 1000;
    $scope.firebreakStats = { series: [[]], summaryData: { hoursSinceLastEvent: 0 }};
    $scope.firebreakItems = [];
    $scope.performanceStats = { series: [[]] };
    $scope.performanceBreakdownStats = { series: [[]] };
    $scope.performanceItems = [];
    $scope.capacityStats = { series: [[]] };
    
    $scope.firebreakDomain = [ 'max', 3600*12, 3600*6, 3600*3, 3600, 0 ];
    $scope.firebreakRange = ['#389604', '#68C000', '#C0DF00', '#FCE300', '#FD8D00', '#FF5D00'];
    
    $scope.performanceColors = [ '#A4D966', '#e9b3e8', '#ffcc8e' ];
    $scope.performanceLine = ['#66B22E', '#da7cd8', '#fe8b00'];
    $scope.fakeCapColors = [ '#ABD3F5', '#72AEEB' ];
    
    $scope.capacityLineStipples = [ '2,2', 'none' ];
    $scope.capacityLineColors = [ '#78B5FA', '#2486F8' ];
    
    $scope.capacityLabels = [ $filter( 'translate' )( 'common.l_yesterday' ), $filter( 'translate' )( 'common.l_today' )];
    $scope.performanceLabels = [ $filter( 'translate' )( 'common.l_1_hour' ), $filter( 'translate' )( 'common.l_now' )];
    
    $scope.activitiesReturned = function( list ){
        $scope.activities = list.events;
    };
    
    $scope.firebreakReturned = function( data ){
        $scope.firebreakStats = data;
        $scope.firebreakItems = [{ number: data.calculated[0].count, description: $filter( 'translate' )( 'status.desc_firebreak' )}];
    };
    
    $scope.performanceReturned = function( data ){
        $scope.performanceStats = data;
        $scope.performanceItems = [{number: data.calculated[0].dailyAverage, description: $filter( 'translate' )( 'status.desc_performance' )}];
    };
    
    $scope.perfBreakdownReturned = function( data ){
        $scope.performanceBreakdownStats = data;
    };
    
    $scope.capacityReturned = function( data ){
        $scope.capacityStats = data;
        
//        var parts = $byte_converter.convertBytesToString( data.calculated[1].total );
        var parts = $byte_converter.convertBytesToString( data.series[1].datapoints[ data.series[1].datapoints.length - 1 ].y );
        parts = parts.split( ' ' );
        
        var num = parseFloat( parts[0] );
        $scope.capacityItems = [{number: data.calculated[0].ratio, description: $filter( 'translate' )( 'status.desc_dedup_ratio' ), separator: ':'},
            {number: num, description: $filter( 'translate' )( 'status.desc_capacity_used' ), suffix: parts[1]}];
    };
    
    // this callback creates the tooltip element
    // which will get constructed ultimately by treemap
    $scope.setFirebreakToolipText = function( data ){
        
        var units = '';
        var value = $scope.transformFirebreakTime( data.secondsSinceLastFirebreak );
        
        if ( data.secondsSinceLastFirebreak === 0 ){
            var str = '<div><div style="font-weight: bold;font-size: 11px;">' + data.context.name + '</div><div style="font-size: 10px;">' + $filter( 'translate' )( 'status.tt_firebreak_never' ) + '</div></div>';
            return str;
        }
        // set the text as years
        if ( value > 365*24*60*60 ){
            value = Math.floor( value / (365*60*60*24) );
            units = $filter( 'translate' )( 'common.years' );
        }
        // set the text as months
        else if ( value > 30*24*60*60 ){
            value = Math.floor( value / (30*60*60*24) );
            units = $filter( 'translate' )( 'common.months' );
        }
        // set as days
        else if ( value > 24*60*60 ){
            value = Math.floor( value / (24*60*60) )
            units = $filter( 'translate' )( 'common.days' );
        }
        // set the text as hours
        else if ( value > (60*60) ){
            value = Math.floor( value / (60*60) )
            units = $filter( 'translate' )( 'common.hours' );            
        }
        else {
            value = Math.floor( value / 60 );
            units = $filter( 'translate' )( 'common.minutes' );
        }
        
        if ( angular.isDefined( data.context ) ){
            
            var str = '<div><div style="font-weight: bold;font-size: 11px;">' + data.context.name + '</div><div style="font-size: 10px;">' + $filter( 'translate' )( 'status.tt_firebreak', { value: value,units: units} ) + '</div></div>';
            return str;
        }
        
        return '';
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
    
    $scope.getActivityClass = function( category ){
        return $activity_service.getClass( category );
    };
    
    $scope.getActivityCategoryString = function( category ){
        return $activity_service.getCategoryString( category );
    };
    
    $scope.getTimeAgo = function( time ){
        return $time_converter.convertToTimePastLabel( time );
    };

    $scope.transformFirebreakTime = function( value ){
        
        if ( value === 0 ){
            return 0;
        }
        
        var nowSeconds = (new Date()).getTime() / 1000;
        
        var val = nowSeconds - value;
        
        return val;
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
             Math.round( ((new Date()).getTime() - (1000*60*60*24))/1000 ),
             Math.round( (new Date()).getTime() / 1000 ) );
    
        return filter;
    };
    
    var buildPerformanceFilter = function(){
        var filter = StatQueryFilter.create( [],
            [ StatQueryFilter.SHORT_TERM_PERFORMANCE ],
            Math.round( ((new Date()).getTime() - (1000*60*60*24))/1000 ),
            Math.round( (new Date()).getTime() / 1000 ) );
        
        return filter;
    };
    
    var buildPerformanceBreakdownFilter = function(){
        var filter = StatQueryFilter.create( [],
            [ StatQueryFilter.PUTS, StatQueryFilter.GETS, StatQueryFilter.SSD_GETS ],
            Math.round( ((new Date()).getTime() - (1000*60*60*24))/1000 ),
            Math.round( (new Date()).getTime() / 1000 ) );
        
        return filter;
    };
    
    var buildCapacityFilter = function(){
        var filter = StatQueryFilter.create( [],
            [ StatQueryFilter.PHYSICAL_CAPACITY, StatQueryFilter.LOGICAL_CAPACITY ],
            Math.round( ((new Date()).getTime() - (1000*60*60*24))/1000 ),
            Math.round( (new Date()).getTime() / 1000 ) );
        
        return filter;
    };
                                                            
    // cleanup the pollers
    $scope.$on( '$destroy', function(){
        $interval.cancel( firebreakInterval );
        $interval.cancel( performanceInterval );
        $interval.cancel( perfBreakdownInterval );
        $interval.cancel( capacityInterval );
        $interval.cancel( activityInterval );
    });
    
    var init = function(){
        
        if ( !$authentication.isAuthenticated ){
            return;
        }
        
        firebreakInterval = $interval( function(){ $stats_service.getFirebreakSummary( buildFirebreakFilter(), $scope.firebreakReturned );}, 60000 );
        performanceInterval = $interval( function(){ $stats_service.getPerformanceSummary( buildPerformanceFilter(), $scope.performanceReturned );}, 60000 );
        capacityInterval = $interval( function(){ $stats_service.getCapacitySummary( buildCapacityFilter(), $scope.capacityReturned );}, 60000 );
        activityInterval = $interval( function(){ $activity_service.getActivities( {points: 15}, $scope.activitiesReturned );}, 60000 );
        perfBreakdownInterval = $interval( function(){ $stats_service.getPerformanceBreakdownSummary( buildPerformanceBreakdownFilter(), $scope.perfBreakdownReturned );}, 60000 );

        
        $stats_service.getFirebreakSummary( buildFirebreakFilter(), $scope.firebreakReturned );
        $stats_service.getPerformanceSummary( buildPerformanceFilter(), $scope.performanceReturned );
        $stats_service.getPerformanceBreakdownSummary( buildPerformanceBreakdownFilter(), $scope.perfBreakdownReturned );
        $stats_service.getCapacitySummary( buildCapacityFilter(), $scope.capacityReturned );
        $activity_service.getActivities( {points: 15}, $scope.activitiesReturned );
    };
    
    $rootScope.$on( 'fds::authentication_success', function(){
        init();
    });
    
    init();

}]);
