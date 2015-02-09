mockStats = function(){
    
    angular.module( 'statistics' ).factory( '$stats_service', [function(){

        var service = {};

        var getStatsToUse = function( filter ){
            
            var stats = [];
            var volumes = [];
            
            if ( filter.contextList.length === 0 ){
                volumes = JSON.parse( window.localStorage.getItem( 'volumes' ) );
            }
            else {
                volumes = filter.contextList;
            }
            
            
            for( var i = 0; i < volumes.length; i++ ){
                console.log( 'getting: ' + volumes[i].id + '_stats' );
                var raw =  window.localStorage.getItem( volumes[i].id + '_stats' );
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
                
                rz.series[0].datapoints.splice( 0, 0, start, before );
                rz.series[1].datapoints.splice( 0, 0, start, before );
                rz.series[2].datapoints.splice( 0, 0, start, before );
            }
            
            return rz;
        };
        
        service.getFirebreakSummary = function( filter, callback ){
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
                calculated: [{
                    average: 0    
                }]
            };
            
            var stats = getStatsToUse( filter );
            
            if ( !angular.isDefined( stats ) || stats.length === 0 || stats[0] === null ){
                return rz;
            }
            
            var loops = stats[0][time].length;
            
            for ( var i = 0; i < loops; i++ ){
                
                var puts = 0;
                var gets = 0;
                var ssd = 0;
                
                for ( var j = 0; j < stats.length; j++ ){
                    var point = stats[j][time][i];
                    puts += point.PUTS;
                    gets += point.GETS;
                    ssd += point.SSD_GETS;
                }
                
                puts /= (stats.length*60);
                gets /= (stats.length*60);
                ssd /= (stats.length*60);

                if ( time !== 'minute' ) {
                    puts /= 60;
                    gets /=60;
                    ssd /= 60;
                }
                
                var t = stats[0][time][i].time;
                rz.series[0].datapoints.push( {x: t, y: puts} );
                rz.series[1].datapoints.push( {x: t, y: gets} );
                rz.series[2].datapoints.push( {x: t, y: ssd} );
            }
            
            rz = fixTheZeros( filter, rz );
            
            callback( rz );
            
        };

        service.getPerformanceSummary = function( filter, callback ){

        };

        service.getCapacitySummary = function( filter, callback ){      

        };    

        return service;
    }]);
};
