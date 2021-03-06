var StatQueryFilter = {
    SHORT_TERM_CAPACITY_SIGMA: 'STC_SIGMA',
    LONG_TERM_CAPACITY_SIGMA: 'LTC_SIGMA',
    SHORT_TERM_PERFORMANCE_SIGMA: 'STP_SIGMA',
    LONG_TERM_PERFORMANCE_SIGMA: 'LTP_SIGMA',
    LOGICAL_CAPACITY: 'LBYTES',
    PHYSICAL_CAPACITY: 'PBYTES',
    USED_CAPACITY: 'UBYTES',
    SHORT_TERM_PERFORMANCE: 'STP_WMA',
    LONG_TERM_PERFORMANCE: 'LTP_WMA',
    PUTS: 'PUTS',
    GETS: 'GETS',
    SSD_GETS: 'SSD_GETS',
    HDD_GETS: 'HDD_GETS',
    
    TYPE_VOLUME: 'VOLUME',
    TYPE_NODE: 'NODE',
    TYPE_SERVICE: 'SERVICE',
    TYPE_USER: 'USER',
    TYPE_TENANT: 'TENANT',
    TYPE_DOMAIN: 'DOMAIN',
    
    create: function( contextList, seriesTypes, startTime, endTime, datapoints ){
        
        var val = { 
            seriesType: seriesTypes,
            range: {
                start: startTime,
                end: endTime
            }
        };
        
        if ( angular.isDefined( contextList ) && 
            contextList.length > 0 ){
            
            val.contexts = contextList;
        }
        
        if ( angular.isDefined( datapoints ) ){
            val.points = datapoints;
        }
        
        return val;
    }
    
};
