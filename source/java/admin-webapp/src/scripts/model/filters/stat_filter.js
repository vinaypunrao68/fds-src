var StatQueryFilter = {
    SHORT_TERM_CAPACITY_SIGMA: 'STC_SIGMA',
    LONG_TERM_CAPACITY_SIGMA: 'LTC_SIGMA',
    SHORT_TERM_PERFORMANCE_SIGMA: 'STP_SIGMA',
    LONG_TERM_PERFORMANCE_SIGMA: 'LTP_SIGMA',
    LOGICAL_CAPACITY: 'PBYTES',
    PHYSICAL_CAPACITY: 'LBYTES',
    SHORT_TERM_PERFORMANCE: 'STP_WMA',
    LONG_TERM_PERFORMANCE: 'LTP_WMA',
    PUTS: 'PUTS',
    GETS: 'GETS',
    SSD_GETS: 'SSD_GETS',
    
    create: function( contextList, seriesTypes, startTime, endTime, datapoints ){
        
        var val = { 
            seriesType: seriesTypes,
            contextList: contextList,
            range: {
                start: startTime,
                end: endTime
            }
        };
        
        if ( angular.isDefined( datapoints ) ){
            val.points = datapoints;
        }
        
        return val;
    }
    
};