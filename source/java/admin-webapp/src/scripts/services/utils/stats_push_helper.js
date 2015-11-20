angular.module( 'statistics' ).factory( '$stats_push_helper', [ function(){
    
    var service = {};
    
    service.test = function(){
        return 'TEST';
    };
    
    // Build a time sum list from the names we're looking for
    // initialized to 0's
    service.initTimeSums = function( metricNames ){
        
        var timeSums = [];
        
        // initialize our sum data
        for ( var st = 0; st < metricNames.length; st++ ){
            timeSums.push( {metricName: metricNames[st], sum: 0, itemCount: 0} );
        }
        
        return timeSums;
    };
    
    // unloads the raw data and turns it into an array of points
    service.convertBusDataToPoints = function( newStatList ){
        
        var converted = [];
        
        for ( var i = 0; i < newStatList.length; i++ ){
            var data = newStatList[i].payload;
            data = JSON.parse( data );
            converted.push( data );
        }
        
        return converted;
    };
    
    // take data and sum the like points into the proper
    // time sum array.
    service.sumTheProperPoints = function( timeSums, newStatList ){
        
        // fill in our array with the data we need.
        for ( var i = 0; i < newStatList.length; i++ ){
            
            for ( var nameIt = 0; nameIt < timeSums.length; nameIt++ ){
                
                var timeSum = timeSums[nameIt];
                var statName = timeSum.metricName;
                var newData = newStatList[i];
                
                if ( newData.metricName === statName ){
                    timeSum.sum += parseInt( newData.metricValue );
                    timeSum.itemCount++;
                }
            }
        }
    };
    
    // Put sums into the correct data series
    service.injectSumsIntoSeries = function( timeSums, tempStats, averageValues ){
        
        for ( var seriesIt = 0; seriesIt < tempStats.series.length; seriesIt++ ){
            
            var inSeries = tempStats.series[seriesIt];
            
            for ( var ourDataIt = 0; ourDataIt < timeSums.length; ourDataIt++ ){
                
                var ourData = timeSums[ourDataIt];
                
                if ( inSeries.type === ourData.metricName && ourData.itemCount > 0 ){
                    var d = (new Date()).getTime();
//                    console.log( 'Adding x: ' + d + ' y: ' + ourData.sum );
                    var value = ourData.sum;
                    
                    if ( angular.isDefined( averageValues ) && averageValues === true ){
                        value = ourData.sum / ourData.itemCount;
                    }
                    
                    inSeries.datapoints.push( { x: d, y: value } );
                }
            }
        }
    };
    
    // delete the points that are too old to keep
    service.deleteOldStats = function( tempStats, duration ){
        
        for ( var dSeriesIt = 0; dSeriesIt < tempStats.series.length; dSeriesIt++ ){
            
            var outSeries = tempStats.series[dSeriesIt];
            
            for ( var j = 0; j < outSeries.datapoints.length; j++ ){

                var t = outSeries.datapoints[j].x;

                if ( t < (new Date()).getTime() - duration ){
                    outSeries.datapoints.splice( j, 1 );
                    
                    // we just removed something so... gotta step back
                    if ( j <= outSeries.datapoints.length ){
                        j--;
                    }
                }
            }
        }
    };
    
    // data = the bound series that the chart is using
    // newStatList = the list of stats we need to sort through
    // metricNames = the metric names that we're looking for
    // durationToKeep = how long to keep data in the list - in ms.
    service.push_stat = function( data, newStatList, metricNames, durationToKeep, averageValues ){
        
        // make a temp copy so we don't trigger any changes until we're ready.
        var tempStats = { series: [] };
        tempStats.series = data.series;

        // if we get multiples we need to sum it up
        var timeSums = service.initTimeSums( metricNames );

        // fill in our array with the data we need.
        service.sumTheProperPoints( timeSums, newStatList );

        // put the new data into the right series
        service.injectSumsIntoSeries( timeSums, tempStats, averageValues );

        // get rid of old points.
        service.deleteOldStats( tempStats, durationToKeep );

        data = tempStats;
        
    };
    
    return service;
}]);