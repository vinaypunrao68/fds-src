var StatQueryFilter = {
    SHORT_TERM_CAPACITY_SIGMA: 'STC_SIGMA',
    LONG_TERM_CAPACITY_SIGMA: 'LTC_SIGMA',
    SHORT_TERM_PERFORMANCE_SIGMA: 'STP_SIGMA',
    LONG_TERM_PERFORMANCE_SIGMA: 'LTP_SIGMA',
    
    create: function( contextList, seriesTypes, startTime, endTime ){
        
        this.seriesType = seriesTypes;
        this.contextList = contextList;
        this.range = {
            start: startTime,
            end: endTime
        };
        
        return this;
    }
    
};