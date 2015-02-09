mockStats = function(){
    
    angular.module( 'statistics' ).factory( '$stats_service', [function(){

        var service = {};

        service.getFirebreakSummary = function( filter, callback ){
        };

        service.getPerformanceBreakdownSummary = function( filter, callback ){
            
            var time = filter.range.end - filter.range.start;
            
            var stats = getStatsToUse( filter );
            
            var rz = {
                metadata: [],
                series: [{
                            datapoints: [{
                                x: 0,
                                y: 0
                            }],
                            type: 'PUTS'
                        },
                        {
                            datapoints: [{
                            }],
                            type: 'GETS'
                        },
                        {
                            datapoints: [{
                            }],
                            type: 'SSD_GETS'
                        }],
                calculate: [
                    {
                        average: 0
                    }
                ]
            };
            
            
        };

        service.getPerformanceSummary = function( filter, callback ){

        };

        service.getCapacitySummary = function( filter, callback ){      

        };    

        return service;
    }]);
};
