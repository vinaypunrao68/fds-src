angular.module( 'volumes' ).controller( 'viewVolumeController', ['$scope', '$volume_api', '$snapshot_service', '$stats_service', '$byte_converter', '$filter', '$interval', '$rootScope', '$media_policy_helper', '$translate', '$time_converter', '$qos_policy_helper', '$timeline_policy_helper', function( $scope, $volume_api, $snapshot_service, $stats_service, $byte_converter, $filter, $interval, $rootScope, $media_policy_helper, $translate, $time_converter, $qos_policy_helper, $timeline_policy_helper ){
    
    var translate = function( key ){
        return $filter( 'translate' )( key );
    };
    
    $scope.snapshots = [];
    $scope.ranges = [];
    $scope.snapshotPolicies = [];
    $scope.timelinePolicies = [];
    $scope.snapshotPolicyDescriptions = [];
    $scope.timelinePreset = '';
    
    $scope.thisVolume = {};
    $scope.capacityStats = { series: [] };
    $scope.performanceStats = { series: [] };
    $scope.performanceItems = [];
    $scope.capacityItems = [];
    $scope.capacityLineStipples = [ '2,2', 'none' ];
    $scope.capacityLineColors = [ '#78B5FA', '#2486F8' ];
    $scope.capacityColors = [ '#ABD3F5', '#72AEEB' ];
    $scope.performanceColors = [ '#489AE1', '#4857C4', '#8784DE' ];
    $scope.performanceLine = ['#8784DE', 'white', 'white']; 
    
    $scope.dedupLabel = '';
    $scope.physicalLabel = '';
    $scope.getLabel = '';
    $scope.ssdGetLabel = '';
    $scope.putLabel = '';
    
    $scope.qos = {};
    $scope.dataConnector = {};
    $scope.mediaPolicy = {};
    $scope.mediaPreset = '';
    
    var capacityIntervalId = -1;
    var performanceIntervalId = -1;
    var capacityQuery = {};
    var performanceQuery = {};
    
    $scope.timeRanges = [
        { displayName: '30 Days', value: 1000*60*60*24*30, labels: [translate( 'common.l_30_days' ), translate( 'common.l_15_days' ), translate( 'common.l_today' )] },
        { displayName: '1 Week', value: 1000*60*60*24*7, labels: [ translate( 'common.l_1_week' ), translate( 'common.l_today' ) ] },
        { displayName: '1 Day', value: 1000*60*60*24, labels: [ translate( 'common.l_yesterday' ), translate( 'common.l_today' )] },
        { displayName: '1 Hour', value: 1000*60*60, labels: [translate( 'common.l_1_hour' ), translate( 'common.l_30_minutes' ), translate( 'common.l_now' )] }
    ];
    
    $scope.performanceTimeChoice = $scope.timeRanges[0];
    $scope.capacityTimeChoice = $scope.timeRanges[0];
    
    $scope.setCapacityTooltipText = function( data, i, j ){
        if ( i == 0 ){
            return $filter( 'translate' )( 'status.desc_dedup_capacity' );
        }
        else {
            return $filter( 'translate' )( 'status.desc_pre_dedup_capacity' );
        }
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
    
    $scope.capacityLabelFx = function( data ){
        return $byte_converter.convertBytesToString( data, 1 );
    };
    
    var getCapacityLegendText = function( series, key ){
        
        if ( !angular.isDefined( series ) ){
            return '';
        }
        
        var datapoints =  series.datapoints;
        
        return $byte_converter.convertBytesToString( datapoints[ datapoints.length-1 ].y ) + ' ' +
            $filter( 'translate' )( key );
    };
    
    var getPerformanceLegendText = function( series, key ){
        
        if ( !angular.isDefined( series ) ){
            return '';
        }
        
        var datapoints =  series.datapoints;
        
        return datapoints[ datapoints.length-1 ].y.toFixed( 2 ) + ' ' +
            $filter( 'translate' )( key );
    };
    
    $scope.getSlaLabel = function(){
        
        if ( $scope.qos.sla === 0 ){
            return $filter( 'translate' )( 'common.l_none' );
        }
        
        return $scope.qos.sla;
    };
    
    $scope.getLimitLabel = function(){
        
        if ( $scope.qos.limit === 0 ){
            return $filter( 'translate' )( 'volumes.qos.l_unlimited' );
        }
        
        return $scope.qos.limit;
    };
    
    $scope.getDataTypeLabel = function(){
        
        if ( !angular.isDefined( $scope.dataConnector ) || !angular.isDefined( $scope.dataConnector.type ) ){
            return '';
        }
        
        var firstLetter = $scope.dataConnector.type.substr( 0, 1 ).toUpperCase();
        var theRest = $scope.dataConnector.type.substr( 1 ).toLowerCase();
        
        return firstLetter + theRest;
    };
    
    $scope.deleteVolume = function(){
        
        var confirm = {
            type: 'CONFIRM',
            text: $filter( 'translate' )( 'volumes.desc_confirm_delete' ),
            confirm: function( result ){
                if ( result === false ){
                    return;
                }
                
                $volume_api.delete( $scope.volumeVars.selectedVolume,
                    function(){ 
                        var $event = {
                            type: 'INFO',
                            text: $filter( 'translate' )( 'volumes.desc_volume_deleted' )
                        };

                        $rootScope.$emit( 'fds::alert', $event );
                        $scope.volumeVars.back();
                });
            }
        };
        
        $rootScope.$emit( 'fds::confirm', confirm );
    };

    $scope.formatDate = function( ms ){
        var d = new Date( parseInt( ms ) );
        return d.toString();
    };

    $scope.capacityReturned = function( data ){
        $scope.capacityStats = data;
        
        $scope.dedupLabel = getCapacityLegendText( $scope.capacityStats.series[0], 'volumes.view.desc_dedup_suffix' );
        $scope.physicalLabel = getCapacityLegendText( $scope.capacityStats.series[1], 'volumes.view.desc_logical_suffix' );
        
        var parts = $byte_converter.convertBytesToString( data.calculated[1].total );
        parts = parts.split( ' ' );
        
        var num = parseFloat( parts[0] );
        $scope.capacityItems = [{number: data.calculated[0].ratio, description: $filter( 'translate' )( 'status.desc_dedup_ratio' ), separator: ':'},
            {number: num, description: $filter( 'translate' )( 'status.desc_capacity_used' ), suffix: parts[1]}];
    };
    
    $scope.performanceReturned = function( data ){
        $scope.performanceStats = data;
        $scope.performanceItems = [
            {number: data.calculated[0].average, description: $filter( 'translate' )( 'status.desc_performance' )},
            {number: data.calculated[1].percentage, description: $filter( 'translate' )( 'status.desc_ssd_percent' ), iconClass: 'icon-lightningbolt', iconColor: '#8784DE', suffix: '%' },
            {number: data.calculated[2].percentage, description: $filter( 'translate' )( 'status.desc_hdd_percent' ), iconClass: 'icon-disk', iconColor: '#4857C4', suffix: '%' }
        ];
        $scope.putLabel = getPerformanceLegendText( $scope.performanceStats.series[0], 'volumes.view.l_avg_puts' );
        $scope.getLabel = getPerformanceLegendText( $scope.performanceStats.series[1], 'volumes.view.l_avg_gets' );
        $scope.ssdGetLabel = getPerformanceLegendText( $scope.performanceStats.series[2], 'volumes.view.l_avg_ssd_gets' );
    };
    
    var buildQueries = function(){
        
        var now = new Date();
        
        capacityQuery = StatQueryFilter.create( [$scope.thisVolume], 
            [StatQueryFilter.PHYSICAL_CAPACITY,StatQueryFilter.LOGICAL_CAPACITY], 
            Math.round( (now.getTime() - $scope.capacityTimeChoice.value)/1000 ),
            Math.round( now.getTime() / 1000 ) );
        
        performanceQuery = StatQueryFilter.create( [$scope.thisVolume],
            [StatQueryFilter.PUTS, StatQueryFilter.GETS, StatQueryFilter.SSD_GETS],
            Math.round( (now.getTime() - $scope.performanceTimeChoice.value)/1000 ),
            Math.round( now.getTime() / 1000 ) );
    };
    
    var pollCapacity = function(){
        $stats_service.getCapacitySummary( capacityQuery, $scope.capacityReturned );
    };
    
    var pollPerformance = function(){
        $stats_service.getPerformanceBreakdownSummary( performanceQuery, $scope.performanceReturned );
    };
    
    $scope.$watch( 'capacityTimeChoice', function(){
        buildQueries();
        pollCapacity();
    });
    
    $scope.$watch( 'performanceTimeChoice', function(){
        buildQueries();
        pollPerformance();
    });
    
    var initSnapshotSettings = function(){
        
        if ( !angular.isDefined( $scope.volumeVars.selectedVolume ) ){
            return;
        }
        
        return $volume_api.getSnapshotPoliciesForVolume( $scope.volumeVars.selectedVolume.id, function( realPolicies ){

            var notTimelinePolicies = [];
            var timelinePolicies = [];

            for ( var i = 0; i < realPolicies.length; i++ ){
                if ( realPolicies[i].name.indexOf( '_TIMELINE_' ) === -1 ){
                    notTimelinePolicies.push( realPolicies[i] );
                }
                else {
                    timelinePolicies.push( realPolicies[i] );
                }
            }

            $scope.snapshotPolicies = notTimelinePolicies;
            $scope.timelinePolicies = {
                continuous: $scope.thisVolume.commit_log_retention,
                policies: timelinePolicies
            };
        });
    };
    
    var initTimeline = function(){
        
        $scope.ranges = [];
        
        for ( var i = 0; i < $scope.snapshots.length; i++ ){
            
            var range = {
                min: new Date( $scope.snapshots[i].creation )
            };
            
            $scope.ranges.push( range );
        }
        
        // create teh continuous range
        var now = (new Date()).getTime();
        $scope.ranges.push( { min: (now - $scope.thisVolume.commit_log_retention*1000), max: now, pwidth: 15 } );
        $scope.thisVolume.timelineTime = now;
    };    
    
    var initQosSettings = function(){
        $scope.qos.sla = $scope.thisVolume.sla;
        $scope.qos.limit = $scope.thisVolume.limit;
        $scope.qos.priority = $scope.thisVolume.priority;
        $scope.mediaPolicy = $media_policy_helper.convertRawToObjects( $scope.thisVolume.mediaPolicy );
        $scope.mediaPreset = $qos_policy_helper.convertRawToPreset( $scope.qos ).label;
    };
    
    /**
    * This method looks through the timeline policies and tries to make English predicate from the information
    */
    var initSnapshotDescriptions = function(){
        
        $scope.timelinePreset = $timeline_policy_helper.convertRawToPreset( $scope.timelinePolicies.policies ).label;
        
        // must be in this order:  continuous = 0, daily = 1, weekly = 2, monthly = 3, yearly = 4
        for ( var i = 0; i < $scope.timelinePolicies.policies.length; i++ ){
            
            var policy = $scope.timelinePolicies.policies[i];
            var value = $filter( 'translate' )( 'volumes.snapshot.desc_for', { time: $time_converter.convertToTime( policy.retention * 1000, 0 ) } ).toLowerCase();
            
            switch( policy.recurrenceRule.FREQ ){
                    
                case 'DAILY':
                    // figure out the time...
                    var time = parseInt( policy.recurrenceRule.BYHOUR[0] );
                    var am = true;
                    
                    if ( time > 11 ){
                        am = false;
                    }
                    
                    time %= 12;
                    
                    if ( time === 0 ){
                        time = 12;
                    }
                    
                    if ( am === true ){
                        time += 'am';
                    }
                    else {
                        time += 'pm';
                    }
                    
                    $scope.snapshotPolicyDescriptions[1] = {
                        label: $filter( 'translate' )( 'volumes.snapshot.l_daily' ),
                        predicate: $filter( 'translate' )( 'volumes.snapshot.desc_at', { time: time } ),
                        value: value
                    };
                    break;
                case 'WEEKLY':
                    $scope.snapshotPolicyDescriptions[2] = {
                        label: $filter( 'translate' )( 'volumes.snapshot.l_weekly' ),
                        predicate: $filter( 'translate' )( 'volumes.snapshot.desc_plural_days', { day: $timeline_policy_helper.convertAbbreviationToString( policy.recurrenceRule.BYDAY[0] ) } ),
                        value: value
                    };
                    break;
                case 'MONTHLY':
                    
                    var predicate = '';
                    
                    // means that we set first or last day
                    if ( angular.isDefined( policy.recurrenceRule.BYMONTHDAY ) ){
                        if ( parseInt( policy.recurrenceRule.BYMONTHDAY[0] ) === -1 ){
                            predicate = $filter( 'translate' )( 'volumes.snapshot.l_last_day_of_the_month' );
                        }
                        else {
                            predicate = $filter( 'translate' )( 'volumes.snapshot.l_first_day_of_the_month' );
                        }
                    }
                    // we must have set first or last week
                    else if ( angular.isDefined( policy.recurrenceRule.BYDAY ) ){
                        
                        // last week of the month
                        if ( policy.recurrenceRule.BYDAY[0].indexOf( '-1' ) !== -1 ){
                            predicate = $filter( 'translate' )( 'volumes.snapshot.l_last_weekly' );
                        }
                        else {
                            predicate = $filter( 'translate' )( 'volumes.snapshot.l_first_weekly' );
                        }
                    }
                    
                    $scope.snapshotPolicyDescriptions[3] = {
                        label: $filter( 'translate' )( 'volumes.snapshot.l_monthly' ),
                        predicate: predicate,
                        value: value
                    };
                    break;
                case 'YEARLY':
                    $scope.snapshotPolicyDescriptions[4] = {
                        label: $filter( 'translate' )( 'volumes.snapshot.l_yearly' ),
                        predicate: $timeline_policy_helper.convertMonthNumberToString( parseInt( policy.recurrenceRule.BYMONTH[0] ) ),
                        value: value
                    };
                    break;
                default:
                    break;
            }
        }
        
        $scope.snapshotPolicyDescriptions[0] = {
            label: $filter( 'translate' )( 'volumes.l_continuous' ),
            predicate: $filter( 'translate' )( 'volumes.snapshot.l_kept' ),
            value: $filter( 'translate' )( 'volumes.snapshot.desc_for', { time: $time_converter.convertToTime( $scope.timelinePolicies.continuous * 1000, 0 ).toLowerCase() } )
        };
    };
    
    // gets the volume info and regenerates the screen
    var initializeVolume = function(){
        
        if ( !angular.isDefined( $scope.volumeVars.selectedVolume ) ){
            return;
        }
        
        $volume_api.getSnapshots( $scope.volumeVars.selectedVolume.id, function( data ){ 
            $scope.snapshots = data;
            initTimeline();
        });

        $scope.thisVolume = $scope.volumeVars.selectedVolume;

        $scope.dataConnector = $scope.thisVolume.data_connector;

        initQosSettings();

        initSnapshotSettings().then( function(){
            // must come after the snapshot settings initialization
            initSnapshotDescriptions();
        });

        buildQueries();

        capacityIntervalId = $interval( pollCapacity, 60000 );
        performanceIntervalId = $interval( pollPerformance, 60000 );
        pollCapacity();
        pollPerformance();        
    };
    
    // when we get shown, get all the snapshots and policies.  THen do the chugging
    // to display the summary and set the hidden forms.
    $scope.$watch( 'volumeVars.viewing', function( newVal ){

        if ( newVal === true ){
            initializeVolume();   
        }
        else {
            $interval.cancel( capacityIntervalId );
            $interval.cancel( performanceIntervalId );
            $scope.$broadcast( 'fds::cancel_editing' );
        }
    });
    
    // when we're done editing we should refresh the screen as well.
    $scope.$watch( 'volumeVars.editing', function( newVal, oldVal ){
        
        if ( newVal === false ){
            initializeVolume();
        }
    });
    
    $scope.edit = function(){
        
        $scope.volumeVars.editing = true;
        $scope.volumeVars.next( 'editVolume' );
    };
    
}]);
