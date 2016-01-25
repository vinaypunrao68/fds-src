angular.module( 'status' ).controller( 'statusController', ['$scope', '$activity_service', '$interval', '$authorization', '$authentication', '$stats_service', '$filter', '$timeout', '$byte_converter', '$time_converter', '$rootScope', '$state', function( $scope, $activity_service, $interval, $authorization, $authentication, $stats_service, $filter, $timeout, $byte_converter, $time_converter, $rootScope, $state ){

    $scope.healthStatus = [{number: 'Excellent'}];
    
    var firebreakInterval = -1;
//    var performanceInterval = -1;
    var capacityInterval = -1;
    var activityInterval = -1;
    var perfBreakdownInterval = -1;
    var healthInterval = -1;
    
    $scope.health = {};
    $scope.activities = [];
    $scope.firebreakMax = 1440;
    $scope.minArea = 1000;
    $scope.firebreakStats = { series: [[]], summaryData: { hoursSinceLastEvent: 0 }};
    $scope.firebreakItems = [];
    $scope.performanceStats = { series: [[]] };
    $scope.performanceBreakdownStats = { series: [[]] };
    $scope.performanceItems = [];
    $scope.capacityItems = [];
    $scope.capacityStats = { series: [[]] };
    $scope.capacityLimit = 100000;
    
    $scope.firebreakDomain = [ 'max', 3600*12, 3600*6, 3600*3, 3600, 0 ];
    $scope.firebreakRange = ['#389604', '#68C000', '#C0DF00', '#FCE300', '#FD8D00', '#FF5D00'];
    
    $scope.performanceColors = [ '#489AE1', '#4857C4', '#8784DE' ];
    $scope.performanceLine = ['#8784DE', 'white', 'white'];
    $scope.fakeCapColors = [ '#ABD3F5', '#72AEEB' ];
    
    $scope.capacityLineStipples = [ '2,2', 'none' ];
    $scope.capacityLineColors = [ '#78B5FA', '#2486F8' ];
    
    $scope.capacityLabels = [ $filter( 'translate' )( 'common.l_yesterday' ), $filter( 'translate' )( 'common.l_today' )];
    $scope.performanceLabels = [ $filter( 'translate' )( 'common.l_1_hour' ), $filter( 'translate' )( 'common.l_now' )];
    
    $scope.goToDebug = function(){
        
        $state.transitionTo( 'homepage.debug' );
    };
    
    /**
    This is a series of methods that make it easy for error paths to try and re-start the polling cycle
    that is now handled with timeout's instead of intervals.
    **/
    var startFirebreakSummary = function(){
        
        $timeout.cancel( firebreakInterval );
        
        $stats_service.getFirebreakSummary( buildFirebreakFilter(), $scope.firebreakReturned, 
            function(){ firebreakInterval = $timeout( startFirebreakSummary, 60000 ); } );
    };
    
    var startPerformanceBreakdownSummary = function(){
        
        $timeout.cancel( perfBreakdownInterval );
        
        $stats_service.getPerformanceBreakdownSummary( buildPerformanceBreakdownFilter(), $scope.perfBreakdownReturned, 
            function(){ perfBreakdownInterval = $timeout( startPerformanceBreakdownSummary, 60000 ); });
    };
    
    var startCapacitySummary = function(){
        
        $timeout.cancel( capacityInterval );
        
        $stats_service.getCapacitySummary( buildCapacityFilter(), $scope.capacityReturned, 
            function(){ capacityInterval = $timeout( startCapacitySummary, 60000 );} );
    };
    
    var startActivityFetching = function(){
        
        $timeout.cancel( activityInterval );
        
        $activity_service.getActivities( {points: 10}, $scope.activitiesReturned, 
            function(){ activityInterval = $timeout( startActivityFetching, 60000 );} );
    };
    
    var startSystemHealthFetching = function(){
        
        $timeout.cancel( healthInterval );
        
        $activity_service.getSystemHealth( $scope.healthReturned, 
            function(){ healthInterval = $timeout( startSystemHealthFetching, 60000 ); } );
    };
    
    /**
    Here are the handlers for when the data is returned.
    **/
    $scope.healthReturned = function( data ){
    
        for ( var i = 0; i < data.status.length; i++ ){
            data.status[i].message = $filter( 'translate' )( 'status.' + data.status[i].message );
        }
        
        $scope.health = data;
        
        $scope.healthStatus = [{ number: $filter( 'translate' )( 'status.l_' + data.overall.toLowerCase() )}];
        
        healthInterval = $timeout( startSystemHealthFetching, 60000 );
    };
    
    $scope.activitiesReturned = function( response ){
        var list = response;
        
        $scope.activities = list;
        activityInterval = $timeout( startActivityFetching, 60000 );
    };
    
    $scope.firebreakReturned = function( response ){
        var data = response;
        $scope.firebreakStats = data;
        
        $scope.firebreakItems = [{ number: data.calculated[0].count, description: $filter( 'translate' )( 'status.desc_firebreak' )}];
        
        firebreakInterval = $timeout( startFirebreakSummary, 60000 );
    };
    
    $scope.perfBreakdownReturned = function( response ){
        var data = response;
        $scope.performanceBreakdownStats = data;
        $scope.performanceBreakdownItems = [{number: data.calculated[0].average, description: $filter( 'translate' )( 'status.desc_performance' )}];
        
        perfBreakdownInterval = $timeout( startPerformanceBreakdownSummary, 60000 );
    };
    
    $scope.capacityReturned = function( response ){
        
        var data = response;
        
        // when the data gets here, it could be in the wrong order.  We need it to be
        // LBYTES first, UBYTES second.
        if ( data.series.length > 1 && data.series[0].type !== 'LBYTES' && data.series[1].type === 'LBYTES' ){
            var tmp = data.series[0];
            data.series[0] = data.series[1];
            data.series[1] = tmp;
        }
        
        $scope.capacityStats = data;
        
        var calculatedValues = data.calculated;
        var secondsToFull = 0;
        var totalCapacity = 0; 
        var capacityUsed = 0;
        var percentUsed = 0;
        var dedupRatio = 0;
        
        for ( var i = 0; i < calculatedValues.length; i++ ){
            var values = calculatedValues[i];
            
            if ( angular.isDefined( values['toFull'] ) ){
                secondsToFull = values['toFull'];
            }
            else if ( angular.isDefined( values['ratio'] ) ){
                dedupRatio = values['ratio'];
            }
            else if ( angular.isDefined( values['total'] )){
                capacityUsed = values['total'];
            }
            else if ( angular.isDefined( values['totalCapacity'] )){
                totalCapacity = values['totalCapacity'];
            }
        }
        
//        var parts = $byte_converter.convertBytesToString( data.calculated[1].total );
        var parts = $byte_converter.convertBytesToString( data.series[1].datapoints[ data.series[1].datapoints.length - 1 ].y );
        parts = parts.split( ' ' );
        
        var num = parseFloat( parts[0] );
        var percentage = ( capacityUsed / totalCapacity * 100 ).toFixed( 0 );
        
        if ( isNaN( percentage ) ){
            percentage = 0;
        }
        
        $scope.capacityItems = [
            {number: percentage, description: '(' + $byte_converter.convertBytesToString( capacityUsed ) + ' / ' + $byte_converter.convertBytesToString( totalCapacity ) + ')', suffix: '% ' + $filter( 'translate' )( 'status.l_used' ) + '  '},
            {number: dedupRatio, description: $filter( 'translate' )( 'status.desc_dedup_ratio' ), separator: ':'}
        ];
        
        if ( angular.isDefined( secondsToFull )){
            
            var convertedStr = $time_converter.convertToTime( secondsToFull*1000 );
            var parts = convertedStr.split( ' ' );
            
            var fullInfo = {
                number: parseFloat( parts[0] ), 
                description: $filter( 'translate' )( 'status.desc_time_to_full' ), 
                suffix: parts[1].toLowerCase() 
            };
            
            // longer than 3 years
            if ( secondsToFull > (3*365*24*60*60) ){
                fullInfo.number = '>3';
            }

            $scope.capacityItems.push( fullInfo );
            $scope.capacityLimit = totalCapacity;
        }
        
        capacityInterval = $timeout( startCapacitySummary, 60000 );
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
    
    $scope.setPerformanceTooltip = function( series ){
        
        var text ='';
        
        switch ( series.type ){
            case 'GETS':
                text = $filter( 'translate' )( 'status.l_gets' );
                break;
            case 'SSD_GETS':
                text = $filter( 'translate' )( 'status.l_ssd_gets' );
                break;
            case 'PUTS':
                text = $filter( 'translate' )( 'status.l_puts' );
                break;
        }
        
        return text;
    };
    
    $scope.setCapacityTooltipText = function( data, i, j ){
        if ( data.type === 'LBYTES' ){
            return $filter( 'translate' )( 'status.desc_pre_dedup_capacity' );
            
        }
        else {
            return $filter( 'translate' )( 'status.desc_dedup_capacity' );
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
             Math.round( (new Date()).getTime() / 1000 )
        );
    
        filter.useSizeForValue = true;
        filter.mostRecentResults = 1;
        
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
            [ StatQueryFilter.PUTS, StatQueryFilter.HDD_GETS, StatQueryFilter.SSD_GETS ],
            Math.round( ((new Date()).getTime() - (1000*60*60*1))/1000 ),
            Math.round( (new Date()).getTime() / 1000 ) );
        
        return filter;
    };
    
    var buildCapacityFilter = function(){
        var filter = StatQueryFilter.create( [],
            [ StatQueryFilter.USED_CAPACITY, StatQueryFilter.LOGICAL_CAPACITY ],
            Math.round( ((new Date()).getTime() - (1000*60*60*24))/1000 ),
            Math.round( (new Date()).getTime() / 1000 ) );
        
        return filter;
    };
                                                            
    // cleanup the pollers
    $scope.$on( '$destroy', function(){
        $timeout.cancel( firebreakInterval );
//        $interval.cancel( performanceInterval );
        $timeout.cancel( perfBreakdownInterval );
        $timeout.cancel( capacityInterval );
        $timeout.cancel( activityInterval );
        $timeout.cancel( healthInterval );
    });
    
    var init = function(){
        
        if ( !$authentication.isAuthenticated ){
            return;
        }
        
        startFirebreakSummary();
        startPerformanceBreakdownSummary();
        startCapacitySummary();
        startActivityFetching();
        startSystemHealthFetching();
        
    };
    
    $rootScope.$on( 'fds::authentication_success', function(){
        init();
    });
    
    init();

}]);
