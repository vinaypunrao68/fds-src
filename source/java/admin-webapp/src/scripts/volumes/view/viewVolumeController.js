angular.module( 'volumes' ).controller( 'viewVolumeController', ['$scope', '$volume_api', '$snapshot_service', '$stats_service', '$byte_converter', '$filter', '$interval', '$rootScope', function( $scope, $volume_api, $snapshot_service, $stats_service, $byte_converter, $filter, $interval, $rootScope ){
    
    var translate = function( key ){
        return $filter( 'translate' )( key );
    };
    
    $scope.snapshots = [];
    $scope.snapshotPolicies = [];
    
    $scope.thisVolume = {};
    $scope.capacityStats = { series: [] };
    $scope.performanceStats = { series: [] };
    $scope.performanceItems = [];
    $scope.capacityItems = [];
    $scope.capacityLineStipples = [ '2,2', 'none' ];
    $scope.capacityLineColors = [ '#78B5FA', '#2486F8' ];
    $scope.capacityColors = [ '#ABD3F5', '#72AEEB' ];
    $scope.performanceColors = [ '#A4D966' ];
    $scope.performanceLine = ['#66B22E'];   
    
    $scope.dedupLabel = '';
    $scope.physicalLabel = '';
    $scope.iopLabel = '';
    
    $scope.qos = {};
    $scope.dataConnector = {};
    
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
        
        return datapoints[ datapoints.length-1 ].y + ' ' +
            $filter( 'translate' )( key );
    };
    
    $scope.deleteSnapshot = function( snapshot ){
        
        $volume_api.deleteSnapshot( $scope.volumeVars.selectedVolume.id, snapshot.id, function(){ alert( 'Snapshot deleted successfully.' );} );
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
        $scope.performanceItems = [{number: data.calculated[0].dailyAverage, description: $filter( 'translate' )( 'status.desc_performance' )}];
        $scope.iopLabel = getPerformanceLegendText( $scope.performanceStats.series[0], 'volumes.view.desc_iops_capacity' );
    };
    
    var buildQueries = function(){
        
        var now = new Date();
        
        capacityQuery = StatQueryFilter.create( [$scope.volume], 
            [StatQueryFilter.LOGICAL_CAPACITY, StatQueryFilter.PHYSICAL_CAPACITY], 
            Math.round( (now.getTime() - $scope.capacityTimeChoice.value)/1000 ),
            Math.round( now.getTime() / 1000 ) );
        
        performanceQuery = StatQueryFilter.create( [$scope.volume],
            [StatQueryFilter.SHORT_TERM_PERFORMANCE],
            Math.round( (now.getTime() - $scope.performanceTimeChoice.value)/1000 ),
            Math.round( now.getTime() / 1000 ) );
    };
    
    var pollCapacity = function(){
        $stats_service.getCapacitySummary( capacityQuery, $scope.capacityReturned );
    };
    
    var pollPerformance = function(){
        $stats_service.getPerformanceSummary( performanceQuery, $scope.performanceReturned );
    };
    
    $scope.$watch( 'capacityTimeChoice', function(){
        buildQueries();
        pollCapacity();
    });
    
    $scope.$watch( 'performanceTimeChoice', function(){
        buildQueries();
        pollPerformance();
    });
    
    // when we get shown, get all the snapshots and policies.  THen do the chugging
    // to display the summary and set the hidden forms.
    $scope.$watch( 'volumeVars.viewing', function( newVal ){

        if ( newVal === true ){
            $volume_api.getSnapshots( $scope.volumeVars.selectedVolume.id, function( data ){ $scope.snapshots = data; } );
            
            $scope.thisVolume = $scope.volumeVars.selectedVolume;
            
            $scope.dataConnector = $scope.thisVolume.data_connector;
            
            $scope.qos.capacity = $scope.thisVolume.sla;
            $scope.qos.limit = $scope.thisVolume.limit;
            $scope.qos.priority = $scope.thisVolume.priority;
            
            $volume_api.getSnapshotPoliciesForVolume( $scope.volumeVars.selectedVolume.id, function( realPolicies ){
                $scope.snapshotPolicies = realPolicies;
            });
            
            buildQueries();
            
            capacityIntervalId = $interval( pollCapacity, 60000 );
            performanceIntervalId = $interval( pollPerformance, 60000 );
        }
        else {
            $interval.cancel( capacityIntervalId );
            $interval.cancel( performanceIntervalId );
        }
    });
    
    $scope.$on( 'fds::snapshot_policy_change', function(){
        
        $volume_api.getSnapshotPoliciesForVolume( $scope.thisVolume.id, function( oldPolicies ){
            $snapshot_service.saveSnapshotPolicies( $scope.thisVolume.id, oldPolicies, $scope.snapshotPolicies );
        });
    });
    
    $scope.$on( 'fds::qos_changed', function(){
        
        if ( !angular.isDefined( $scope.thisVolume.id ) ){
            return;
        }
    
        $scope.thisVolume.sla = $scope.qos.capacity;
        $scope.thisVolume.priority = $scope.qos.priority;
        $scope.thisVolume.limit = $scope.qos.limit;
        
        $volume_api.save( $scope.thisVolume );
    });
    
    $scope.$on( 'fds::data_conenctor_changed', function( newVal, oldVal ){
        
        if ( !angular.isDefined( $scope.thisVolume.id ) || !angular.isDefined( oldVal ) ){
            return;
        }        

        $scope.thisVolume.data_connector = $scope.dataConncetor;
        
        $volume_api.save( $scope.thisVolume );
    });
    
}]);
