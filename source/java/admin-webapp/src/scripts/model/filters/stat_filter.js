var StatQueryFilter = {
    SHORT_TERM_CAPACITY_SIGMA: 'STC_SIGMA',
    LONG_TERM_CAPACITY_SIGMA: 'LTC_SIGMA',
    SHORT_TERM_PERFORMANCE_SIGMA: 'STP_SIGMA',
    LONG_TERM_PERFORMANCE_SIGMA: 'LTP_SIGMA',
    LOGICAL_CAPACITY: 'PBYTES',
    PHYSICAL_CAPACITY: 'LBYTES',
    SHORT_TERM_PERFORMANCE: 'STP_WMA',
    LONG_TERM_PERFORMANCE: 'LTP_WMA',
    
    create: function( contextList, seriesTypes, startTime, endTime ){
        
        var val = { 
            seriesType: seriesTypes,
            contextList: contextList,
            range: {
                start: startTime,
                end: endTime
            }
        };
        
        return val;
    }
    
};