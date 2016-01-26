angular.module( 'volumes' ).controller( 'viewVolumeController', ['$scope', '$volume_api', '$snapshot_service', '$stats_service', '$byte_converter', '$filter', '$timeout', '$rootScope', '$media_policy_helper', '$translate', '$time_converter', '$qos_policy_helper', '$timeline_policy_helper', function( $scope, $volume_api, $snapshot_service, $stats_service, $byte_converter, $filter, $timeout, $rootScope, $media_policy_helper, $translate, $time_converter, $qos_policy_helper, $timeline_policy_helper ){
    
    var translate = function( key ){
        return $filter( 'translate' )( key );
    };
    
    $scope.firebreakModel = [
        { x: ((new Date()).getTime()/1000) - (6*60*60), y: 1 },
        { x: ((new Date()).getTime()/1000) - (10*60*60), y: 1 }
    ];
    
    $scope.firebreakDomain = [ 3600*24, 3600*12, 3600*6, 3600*3, 3600, 0 ];
    $scope.firebreakRange = ['#389604', '#68C000', '#C0DF00', '#FCE300', '#FD8D00', '#FF5D00'];
    $scope.lastTwentyFour = { start: ((new Date()).getTime()/1000) - (24*60*60), end: ((new Date()).getTime()/1000 ) };
    
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
    $scope.capacityLineStipples = [ '2,2'];
    $scope.capacityLineColors = [ '#78B5FA'];
    $scope.capacityColors = [ '#ABD3F5'];
    $scope.performanceColors = [ '#489AE1', '#4857C4', '#8784DE' ];
    $scope.performanceLine = ['#8784DE', 'white', 'white']; 
    
    $scope.logicalLabel = '';
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
    var firebreakIntervalId = -1;
    var capacityQuery = {};
    var performanceQuery = {};
    var firebreakQuery = {};
    var timelinePresets = [];
    var qosPreset = [];
    
    $scope.timeRanges = [
        { displayName: '30 Days', value: 1000*60*60*24*30, labels: [translate( 'common.l_30_days' ), translate( 'common.l_15_days' ), translate( 'common.l_today' )] },
        { displayName: '1 Week', value: 1000*60*60*24*7, labels: [ translate( 'common.l_1_week' ), translate( 'common.l_today' ) ] },
        { displayName: '1 Day', value: 1000*60*60*24, labels: [ translate( 'common.l_yesterday' ), translate( 'common.l_today' )] },
        { displayName: '1 Hour', value: 1000*60*60, labels: [translate( 'common.l_1_hour' ), translate( 'common.l_30_minutes' ), translate( 'common.l_now' )] }
    ];
    
    $scope.performanceTimeChoice = $scope.timeRanges[0];
    $scope.capacityTimeChoice = $scope.timeRanges[0];
    
    $scope.setCapacityTooltipText = function( data, i, j ){
        if ( data.type === 'PBYTES' ){
            return $filter( 'translate' )( 'status.desc_dedup_capacity' );
        }
        else {
            return $filter( 'translate' )( 'status.desc_pre_dedup_capacity' );
        }
    };
    
    $scope.setPerformanceTooltip = function( series ){
        
        var text ='';
        
        switch ( series.type ){
            case 'HDD_GETS':
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
        
        var legend_text = '0 ' + $filter( 'translate' )( key );
        
        if ( angular.isDefined( datapoints ) && datapoints.length > 0 ){
            legend_text = $byte_converter.convertBytesToString( datapoints[ datapoints.length-1 ].y ) + ' ' +
                $filter( 'translate' )( key );
        }
        
        return legend_text;
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
        
        if ( $scope.qos.iopsMin === 0 ){
            return $filter( 'translate' )( 'common.l_none' );
        }
        
        return $scope.qos.iopsMin;
    };
    
    $scope.getLimitLabel = function(){
        
        if ( $scope.qos.iopsMax === 0 ){
            return $filter( 'translate' )( 'volumes.qos.l_unlimited' );
        }
        
        return $scope.qos.iopsMax;
    };
    
    $scope.getDataTypeLabel = function(){
        
        if ( !angular.isDefined( $scope.thisVolume ) || !angular.isDefined( $scope.thisVolume.settings ) ){
            return '';
        }
        
        if ( $scope.thisVolume.settings.type == 'ISCSI' ){
            return 'iSCSI';
        }
        
        var firstLetter = $scope.thisVolume.settings.type.substr( 0, 1 ).toUpperCase();
        var theRest = $scope.thisVolume.settings.type.substr( 1 ).toLowerCase();
        
        return firstLetter + theRest;
    };
    
    $scope.getAllocatedSize = function(){
        
        if ( angular.isDefined( $scope.thisVolume.settings ) && angular.isDefined( $scope.thisVolume.settings.capacity ) ){
            var sizeString = $byte_converter.convertBytesToString( $scope.thisVolume.settings.capacity.value, 0 );
            return sizeString;
        }
        
        return '';
    };

    $scope.formatDate = function( ms ){
        var d = new Date( parseInt( ms ) );
        return d.toString();
    };
    
    /**
    Data return handlers
    **/

    $scope.capacityReturned = function( data ){
        $scope.capacityStats = data;
        
        lbyteSeries = {};
        pbyteSeries = {};
        
        for ( var i = 0; i < $scope.capacityStats.series.length; i++ ){
            
            series = $scope.capacityStats.series[i];
            
            if ( series.type == 'PBYTES' ){
                pbyteSeries = series;
            }
            else if ( series.type == 'LBYTES' ){
                lbyteSeries = series;
            }
        }
        
        $scope.logicalLabel = getCapacityLegendText( lbyteSeries, 'volumes.view.desc_logical_suffix' );
        var lbyteTotal = 0;
        
        if ( angular.isDefined( lbyteSeries ) && angular.isDefined( lbyteSeries.datapoints ) && lbyteSeries.datapoints.length > 0 ){
            lbyteSeries.datapoints[lbyteSeries.datapoints.length - 1].y;
        }
        
        var parts = $byte_converter.convertBytesToString( lbyteTotal );
        parts = parts.split( ' ' );
        
        var num = parseFloat( parts[0] );
        $scope.capacityItems = [{number: num, description: $filter( 'translate' )( 'status.desc_logical_capacity_used' ), suffix: parts[1]}];
        
        capacityIntervalId = $timeout( pollCapacity, 60000 );
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
        
        performanceIntervalId = $timeout( pollPerformance, 60000 );
    };
    
    $scope.firebreakReturned = function( data ){
        
        $scope.firebreakModel = [];
                                                                 
        // transform the raw data into the firebreaktimeline format
        for ( var i = 0; data.series.length > 0 && i < data.series[0].datapoints.length; i++ ){
            
            $scope.firebreakModel.push( data.series[0].datapoints[i] );
        }
        
        $scope.lastTwentyFour = { start: ((new Date()).getTime()/1000) - (24*60*60), end: ((new Date()).getTime()/1000 ) };
        
        firebreakIntervalId = $timeout( pollFirebreak, 60000 );
    };
    
    /**
    Polling functions
    **/
    var pollCapacity = function(){
        
        $timeout.cancel( capacityIntervalId );
        
        var now = new Date();
        
        capacityQuery = StatQueryFilter.create( [$scope.thisVolume], 
            [StatQueryFilter.LOGICAL_CAPACITY], 
            Math.round( (now.getTime() - $scope.capacityTimeChoice.value)/1000 ),
            Math.round( now.getTime() / 1000 ) );
        
        $stats_service.getCapacitySummary( capacityQuery, $scope.capacityReturned, 
            function(){ 
                capacityIntervalId = $timeout( pollCapacity, 60000 ); 
            });
    };
    
    var pollPerformance = function(){

        $timeout.cancel( performanceIntervalId );
        
        var now = new Date();
        
        performanceQuery = StatQueryFilter.create( [$scope.thisVolume],
            [StatQueryFilter.PUTS, StatQueryFilter.HDD_GETS, StatQueryFilter.SSD_GETS],
            Math.round( (now.getTime() - $scope.performanceTimeChoice.value)/1000 ),
            Math.round( now.getTime() / 1000 ) );
        
        $stats_service.getPerformanceBreakdownSummary( performanceQuery, $scope.performanceReturned, 
            function(){ performanceIntervalId = $timeout( pollPerformance, 60000 ); } );
    };
    
    var pollFirebreak = function(){
        
        $timeout.cancel( firebreakIntervalId );
        
        var now = new Date();
        
        firebreakQuery = StatQueryFilter.create( [$scope.thisVolume],
            [ StatQueryFilter.SHORT_TERM_CAPACITY_SIGMA,
             StatQueryFilter.LONG_TERM_CAPACITY_SIGMA,
             StatQueryFilter.SHORT_TERM_PERFORMANCE_SIGMA,
             StatQueryFilter.LONG_TERM_PERFORMANCE_SIGMA],
            Math.round( (now.getTime() - $scope.performanceTimeChoice.value)/1000 ),
            Math.round( now.getTime() / 1000 ) );        
        
        $stats_service.getFirebreakSummary( firebreakQuery, $scope.firebreakReturned, 
            function(){ firebreakIntervalId = $timeout( pollFirebreak, 60000 );} );
    };
    
    /**
    watchers
    **/
    $scope.$watch( 'capacityTimeChoice', function(){
        pollCapacity();
    });
    
    $scope.$watch( 'performanceTimeChoice', function(){
        pollPerformance();
    });
    
    var initSnapshotSettings = function(){
        
        if ( !angular.isDefined( $scope.volumeVars.selectedVolume ) ){
            return;
        }
        
//        return $volume_api.getSnapshotPoliciesForVolume( $scope.volumeVars.selectedVolume.id.uuid, function( realPolicies ){

        var notTimelinePolicies = [];
        var timelinePolicies = [];
        var realPolicies = $scope.volumeVars.selectedVolume.dataProtectionPolicy.snapshotPolicies;

        for ( var i = 0; i < realPolicies.length; i++ ){
            if ( realPolicies[i].type.indexOf( 'SYSTEM_TIMELINE' ) === -1 ){
                notTimelinePolicies.push( realPolicies[i] );
            }
            else {
                timelinePolicies.push( realPolicies[i] );
            }
        }

        $scope.snapshotPolicies = notTimelinePolicies;
        $scope.timelinePolicies = {
            commitLogRetention: $scope.thisVolume.dataProtectionPolicy.commitLogRetention,
            snapshotPolicies: timelinePolicies
        };
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
        $scope.ranges.push( { min: (now - $scope.thisVolume.dataProtectionPolicy.commitLogRetention.seconds*1000), max: now, pwidth: 15 } );
        $scope.thisVolume.timelineTime = now;
    };    
    
    var initQosSettings = function(){
        $scope.qos = $scope.thisVolume.qosPolicy;
        $scope.mediaPolicy = $media_policy_helper.convertRawToObjects( $scope.thisVolume.mediaPolicy );
        $scope.mediaPreset = '';
        
        // get the label right
        for ( var i = 0; i < qosPresets.length; i++ ){
            var qosPreset = qosPresets[i];
            
            if ( qosPreset.iopsMax === $scope.qos.iopsMax &&
                qosPreset.iopsMin === $scope.qos.iopsMin &&
                qosPreset.priority === $scope.qos.priority ){
                $scope.mediaPreset = qosPreset.name;
                break;
            }
        }
        
        if ( $scope.mediaPreset === '' ){
            $scope.mediaPreset = $filter( 'translate' )( 'common.l_custom' );
        }
    };
    
    /**
    * This method looks through the timeline policies and tries to make English predicate from the information
    */
    var initSnapshotDescriptions = function(){
        
        $scope.timelinePreset = '';
        
        for ( var tp = 0; tp < timelinePresets.length; tp++ ){
            var areTheyEqual = $timeline_policy_helper.arePoliciesEqual( timelinePresets[tp].snapshotPolicies, $scope.timelinePolicies.snapshotPolicies );
            
            if ( areTheyEqual === true ){
                $scope.timelinePreset = timelinePresets[tp].name;
                break;
            }
        }
        
        // must be custom
        if ( $scope.timelinePreset === '' ){
            $scope.timelinePreset = $filter( 'translate' )( 'common.l_custom' );
        }
        
//        $scope.timelinePreset = $timeline_policy_helper.convertRawToPreset( $scope.timelinePolicies.policies ).label;
        
        // must be in this order:  continuous = 0, daily = 1, weekly = 2, monthly = 3, yearly = 4
        for ( var i = 0; i < $scope.timelinePolicies.snapshotPolicies.length; i++ ){
            
            var policy = $scope.timelinePolicies.snapshotPolicies[i];
            var value = $filter( 'translate' )( 'volumes.snapshot.desc_for', { time: $time_converter.convertToTime( policy.retentionTime.seconds * 1000, 0 ) } ).toLowerCase();
            
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
            value: $filter( 'translate' )( 'volumes.snapshot.desc_for', { time: $time_converter.convertToTime( $scope.timelinePolicies.commitLogRetention.seconds * 1000, 0 ).toLowerCase() } )
        };
    };
    
    // gets the volume info and regenerates the screen
    var initializeVolume = function(){
        
        if ( !angular.isDefined( $scope.volumeVars.selectedVolume ) ){
            return;
        }
        
        $scope.thisVolume = $scope.volumeVars.selectedVolume;
        
        $volume_api.getSnapshots( $scope.volumeVars.selectedVolume.uid, function( data ){ 
            $scope.snapshots = data;
            initTimeline();
        });

        $volume_api.getQosPolicyPresets( function( presets ){
            
            qosPresets = presets;
            
            initQosSettings();
        });
        
        $volume_api.getDataProtectionPolicyPresets( function( presets ){
            
            timelinePresets = presets;
            
            initSnapshotSettings();
            // must come after the snapshot settings initialization
            initSnapshotDescriptions();
        });

        pollCapacity();
        pollPerformance();        
        pollFirebreak();
    };
    
    // when we get shown, get all the snapshots and policies.  THen do the chugging
    // to display the summary and set the hidden forms.
    $scope.$watch( 'volumeVars.viewing', function( newVal ){

        if ( newVal === true ){
            initializeVolume();   
        }
        else {
            $timeout.cancel( capacityIntervalId );
            $timeout.cancel( performanceIntervalId );
            $timeout.cancel( firebreakIntervalId );
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
