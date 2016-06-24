var StatQueryFilter = {
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
