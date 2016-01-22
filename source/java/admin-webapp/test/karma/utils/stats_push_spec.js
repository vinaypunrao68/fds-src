describe( 'Stats Push Utility Tests', function(){
    
    var stats_push;
    var timeSums;
    var metricNames = ['PUTS', 'GETS', 'SomethingElse'];
    
    var series;
    var s1 = { type: 'PUTS', datapoints: [{x: 0, y:2}]};
    var s2 = { type: 'GETS', datapoints: [{x: 0, y: 4}, {x: 1, y:20}]};
    var s3 = { type: 'SomethingElse', datapoints: [{x: 0, y:200}]};
    series = [s1, s2, s3];
    var tempStats = { series: series };

    
    beforeEach( module( 'statistics' ) );
    
    beforeEach( inject( function( _$stats_push_helper_ ){
        stats_push = _$stats_push_helper_;
    }));
    
    it ( 'just a quick test', function(){
        console.log( 'Does ' + stats_push.test() + ' = TEST? ' + (stats_push.test() == 'TEST'));
        expect( stats_push.test() ).toEqual( 'TEST' );
    });
    
    it ( 'Test the initialization of the sum array', function(){
        
        timeSums = stats_push.initTimeSums( metricNames );
        
        expect( timeSums.length ).toEqual( 3 );
        
        for ( var i = 0; i < metricNames.length; i++ ){
            expect( timeSums[i].metricName ).toEqual( metricNames[i] );
            expect( timeSums[i].sum ).toEqual( 0 );
            expect( timeSums[i].itemCount ).toEqual( 0 );
        }
    });
    
    it ( 'Test that points are properly summed and couneted', function(){
        
        var newData = [
            {metricName: 'PUTS', metricValue: 12},
            {metricName: 'GETS', metricValue: 1},
            {metricName: 'GETS', metricValue: 5},
            {metricName: 'PUTS', metricValue: 6},
            {metricName: 'IOPS', metricValue: 12},
            {metricName: 'BAGELS', metricValue: 12},
            {metricName: 'PUTS', metricValue: 30},
            {metricName: 'HDD_DEATHS', metricValue: 12},
            {metricName: 'IOPS', metricValue: 12},
            {metricName: 'IOPS', metricValue: 12},
            {metricName: 'GETS', metricValue: 3}
        ];
        
        stats_push.sumTheProperPoints( timeSums, newData );
        
        expect( timeSums[0].metricName ).toEqual( 'PUTS' );
        expect( timeSums[0].sum ).toEqual( 48 );
        expect( timeSums[0].itemCount ).toEqual( 3 );
        
        expect( timeSums[1].metricName ).toEqual( 'GETS' );
        expect( timeSums[1].sum ).toEqual( 9 );
        expect( timeSums[1].itemCount ).toEqual( 3 );
        
        expect( timeSums[2].metricName ).toEqual( 'SomethingElse' );
        expect( timeSums[2].sum ).toEqual( 0 );
        expect( timeSums[2].itemCount ).toEqual( 0 );
    });
    
    it ( 'Test that the points are injected into the correct series', function() {
        

        stats_push.injectSumsIntoSeries( timeSums, tempStats );
        
        expect( series[0].datapoints.length ).toEqual( 2 );
        expect( series[1].datapoints.length ).toEqual( 3 );
        expect( series[2].datapoints.length ).toEqual( 1 );
        
        expect( series[0].datapoints[1].y ).toEqual( 48 );
        expect( series[1].datapoints[2].y ).toEqual( 9 );
    });
    
    it ( 'Test that old points are deleted', function(){
        
        stats_push.deleteOldStats( tempStats, 10*60*60 );
        
        expect( series[0].datapoints.length ).toEqual( 1 );
        expect( series[1].datapoints.length ).toEqual( 1 );
        expect( series[2].datapoints.length ).toEqual( 0 );
        
        expect( series[0].datapoints[0].y ).toEqual( 48 );
        expect( series[1].datapoints[0].y ).toEqual( 9 );
    });
});