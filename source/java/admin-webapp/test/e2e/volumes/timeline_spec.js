require( '../utils' );

describe( 'Testing the timeline settings', function(){
    
    it( 'should be able to create a volume without changing any settings', function(){
        login();
        
        createVolume( 'timelineVol', { type: 'object' }, { priority: 3, limit: 500, capacity: 90 } );
        
    });
});