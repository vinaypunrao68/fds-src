require( '../utils' );

describe( 'Testing the timeline settings', function(){
    
    var checkValues = function( volumeName, values ){
        
        var row = element( by.cssContainingText( '.name', volumeName ));
        row.click();
        
        browser.sleep( 200 );
        
        var viewPage = element( by.css( '.view-volume-screen' ));
        var timelinePanel = viewPage.element( by.css( '.protection-policy' ));
        
        // verify the value strings match what was passed in as the expected
        for ( var i = 0; i < values.length; i++ ){
            timelinePanel.all( by.css( '.value-label' )).get( i ).getText( function( txt ){
                expect( txt ).toBe( values[i] );
            });            
        }
    };
    
    it( 'should be able to create a volume with singular time settings', function(){
        login();
        
        createVolume( 'timelineVol', { type: 'object' }, { priority: 3, limit: 500, capacity: 90 },
            [
                {slider: 0, value: 1, unit: 0},
                {slider: 1, value: 1, unit: 1},
                {slider: 2, value: 30, unit: 2},
                {slider: 3, value: 1, unit: 3},
                {slider: 4, value: 16, unit: 4}
            ]);
        
        var row = element( by.cssContainingText( '.name', 'timelineVol' ));
        
        expect( row ).not.toBe( undefined );
        
        checkValues( 'timelineVol', ['1 Day', '3 Days', '1 Week', '30 Days', 'Forever'] );
        
    });
//    
//    it( 'should be able to create a volume with plural units', function(){
//        
//        createVolume( 'pluralTimelineVol', { type: 'object' }, undefined,
//            [
//                {slider: 0, value: 2, unit: 0},
//                {slider: 1, value: 2, unit: 1},
//                {slider: 2, value: 120, unit: 2},
//                {slider: 3, value: 2, unit: 3},
//                {slider: 4, value: 4, unit: 4}
//            ]);
//        
//        var row = element( by.cssContainingText( '.name', 'pluralTimelineVol' ));
//        
//        expect( row ).not.toBe( undefined );
//        
//        checkValues( 'timelineVol', ['2 Days', '2 Weeks', '120 Days', '2 Years', '4 Years'] );        
//
//    });
});