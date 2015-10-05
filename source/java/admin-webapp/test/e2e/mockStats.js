mockStats = function(){
    
    angular.module( 'statistics' ).factory( '$stats_service', [function(){

        var service = {};

        /**
        * From the filter this gathers a list of volumes to include in the computations
        **/
        var getStatsToUse = function( filter ){
            
            var stats = [];
            var volumes = [];
            
            if ( !angular.isDefined( filter.contexts ) || filter.contexts.length === 0 ){
                volumes = JSON.parse( window.localStorage.getItem( 'volumes' ) );
            }
            else {
                volumes = filter.contexts;
            }
            
            
            for( var i = 0; volumes !== null && i < volumes.length; i++ ){
                var raw =  window.localStorage.getItem( volumes[i].uid + '_stats' );
                var item = JSON.parse( raw );
                stats.push( item );
            }
            
            return stats;
        };
        
        /**
        * in the case that there is not enough data, we need to add some zeros
        **/
        var fixTheZeros = function( filter, rz ){
            
            if ( rz.series[0].datapoints.length === 0 ){
                return rz;
            }
            
            var before = { x: rz.series[0].datapoints[0].x, y: 0};
            var start = { x: filter.range.start*1000, y: 0 };
            
            if ( Math.abs( rz.series[0].datapoints[0].x - filter.range.start ) > 60*1000 ){
                
                for ( var i = 0; i < rz.series.length; i++ ){
                    rz.series[i].datapoints.splice( 0, 0, start, before );
                }
            }
            
            return rz;
        };
        
        /**
        * takes the raw stats and buckets them by time.  It also does an operation on the data
        * as specified by the caller
        **/
        var bucketByTime = function( stats, time ){
            
            // need to match up datapoints in buckets
            var buckets = [];
            var keyList = [];
            
            for ( var i = 0; i < stats.length; i++ ){
                
                for ( var j = 0; stats[1] !== null && j < stats[i][time].length; j++ ){
                    
                    var stat = stats[i][time][j];
                    var timeInSeconds = Math.floor( stat.time / 1000 );
                    
                    var bucket = buckets[timeInSeconds];
                    
                    if ( bucket === null || !angular.isDefined( bucket ) ){
                        buckets[timeInSeconds] = stat;
                        keyList.push( timeInSeconds );
                        continue;
                    }
                    
                    // if the bucket does exist
                    var tPuts = stat.PUTS/60;
                    var tGets = stat.GETS/60;
                    var tSsd = stat.SSD_GETS/60;
                    
                    if ( time !== 'minute' ){
                        tPuts /= 60;
                        tGets /= 60;
                        tSsd /= 60;
                    }
                    
                    bucket.PUTS += tPuts;
                    bucket.GETS += tGets;
                    bucket.SSD_GETS += tSsd;
                    bucket.LBYTES += stat.LBYTES;
                    bucket.PBYTES += stat.PBYTES;
                }
            }
            
            return { buckets: buckets, keyList: keyList };
        };
        
        service.getFirebreakSummary = function( filter, callback ){
            
            var rz = {
                metadata: [],
                series: [],
                calculated: [{count: 0}]
            };
            
            
            var volumes = JSON.parse( window.localStorage.getItem( 'volumes' ) );
            
            if ( angular.isDefined( filter.contexts ) && filter.contexts.length > 0 ){
                volumes = [];
                
                for ( var i = 0; i < filter.contexts.length; i++ ){
                    volumes.push( filter.contexts[i] );
                }
            }
            
            var fbCount = 0;
            
            for ( var i = 0; volumes!== null && i < volumes.length; i++ ){
                
                var fbEvent = JSON.parse( window.localStorage.getItem( volumes[i].uid + '_fb' ) );
                var point = { x: volumes[i].status.currentUsage.size, y: 3600*24 };
                
                if ( angular.isDefined( fbEvent ) && fbEvent !== null ){
                    point.y = fbEvent.time;
                }
                
                if ( point.y > 24*60*60 ){
                    fbCount++;
                }
                
                if ( !angular.isDefined( filter.useSizeForValue )){
                    
                    var temp = point.y;
                    point.y = point.x;
                    point.x = temp;
                }
                
                var series = {
                    datapoints: [point],
                    context: volumes[i],
                    type: 'Firebreak'
                };
                
                rz.series.push( series );
            }// i
            
            rz.calculated[0].count = fbCount;
            
            if ( angular.isFunction( callback ) ){
                callback( rz );
            }
            
            return rz;
        };

        service.getPerformanceBreakdownSummary = function( filter, callback ){
            
            var time = filter.range.end - filter.range.start;
            
            if ( time <= 1000*60*60 ){
                time = 'minute';
            }
            else {
                time = 'hour';
            }
            
            var rz = {
                metadata: [],
                series: [{
                            datapoints: [],
                            type: 'PUTS'
                        },
                        {
                            datapoints: [],
                            type: 'GETS'
                        },
                        {
                            datapoints: [],
                            type: 'SSD_GETS'
                        }],
                calculated: [
                    {
                        average: 0    
                    },
                    {
                        percentage: 0
                    },
                    {
                        percentage: 0
                    }]
            };
            
            var stats = getStatsToUse( filter );
            
            if ( !angular.isDefined( stats ) || stats.length === 0 || stats[0] === null ){
                return rz;
            }
            
            // need to match up datapoints in buckets            
            var bucketObj = bucketByTime( stats, time );
            var buckets = bucketObj.buckets;
            var keyList = bucketObj.keyList;
            
            // now go through the buckets to populate the data form
            var tSum = 0;
            var ssdSum = 0;
            var hddSum = 0;
            
            for ( var k = 0; k < keyList.length; k++ ){
                var bStat = buckets[ keyList[k] ];
                rz.series[0].datapoints.push( { x: bStat.time, y: bStat.PUTS } );
                rz.series[1].datapoints.push( { x: bStat.time, y: bStat.GETS } );
                rz.series[2].datapoints.push( { x: bStat.time, y: bStat.SSD_GETS } );
                
                ssdSum += bStat.SSD_GETS;
                hddSum += bStat.GETS;
                
                tSum += bStat.PUTS + bStat.GETS + bStat.SSD_GETS;
            }
            
            rz.calculated[0].average = ( tSum / keyList.length ).toFixed( 2 );
            rz.calculated[1].percentage = (( ssdSum / (ssdSum+hddSum) ) * 100 ).toFixed( 0 );
            rz.calculated[2].percentage = (( hddSum / (ssdSum+hddSum) ) * 100 ).toFixed( 0 );
            
            rz = fixTheZeros( filter, rz );
            
            callback( rz );
            
        };

        service.getPerformanceSummary = function( filter, callback ){

        };

        service.getCapacitySummary = function( filter, callback ){      
            
            var rz = {
                metadata: [],
                series: [
                    {
                        datapoints: [],
                        type: 'LBYTES'
                    },
                    {
                        datapoints: [],
                        type: 'PBYTES'
                    }
                ],
                calculated: [
                    {ratio: 0},
                    {total: 0},
                    {totalCapacity: 0 },
                    {toFull: 365*24*60*60 }
                ]
            };
            
            var time = filter.range.end - filter.range.start;
            
            if ( time <= 1000*60*60 ){
                time = 'minute';
            }
            else {
                time = 'hour';
            }
            
            var stats = getStatsToUse( filter );
            var nodes = window.localStorage.getItem( 'nodes' );
            
            if ( !angular.isDefined( stats ) || stats.length === 0 || stats[0] === null ){
                return rz;
            }
            
            if ( nodes === null ){
                nodes = ['node'];
            }
            else {
                nodes = JSON.parse( nodes );
            }
            
            var bucketObj = bucketByTime( stats, time );
            
            // put the data into the result
            var pSum = 0;
            var lSum = 0;
            var first = 0;
            var last = 0;
            
            for ( var i = 0; i < bucketObj.keyList.length; i++ ){
                
                var stat = bucketObj.buckets[ bucketObj.keyList[i] ];
                
                rz.series[0].datapoints.push( { x: stat.time, y: stat.LBYTES } );
                rz.series[1].datapoints.push( { x: stat.time, y: stat.PBYTES } );
                
                pSum += stat.PBYTES;
                lSum += stat.LBYTES;
                
                if ( i === 0 ){
                    first = stat.PBYTES;
                }
                
                if ( (i+1) === bucketObj.keyList.length ){
                    last = stat.PBYTES;
                }
            }
            
            rz.calculated[0].ratio = ( lSum / pSum ).toFixed( 2 );
            
            if ( rz.calculated[0].ratio > 100000 ){
                rz.calculated[0].ratio = 'unknown';
            }
            
            rz.calculated[1].total = pSum;
            rz.calculated[2].totalCapacity = Math.pow( 1024, 4 )*nodes.length;
            
            var slope = ( last - first ) / (( bucketObj.keyList[ bucketObj.keyList.length - 1] ) - bucketObj.keyList[0] );
            
            rz.calculated[3].toFull = (Math.pow( 1024, 4 ) / (100*slope)).toFixed( 0 );
            
            rz = fixTheZeros( filter, rz );
            
            if ( angular.isFunction( callback ) ){
                callback( rz );
            }
            
            return rz;
        };    

        return service;
    }]);
};
