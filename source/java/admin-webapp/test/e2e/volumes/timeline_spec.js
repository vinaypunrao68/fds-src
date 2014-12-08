require( '../utils' );

describe( 'Testing the timeline settings', function(){
    
    var checkValues = function( volumeName, values ){
        
        var row = element( by.cssContainingText( '.name', volumeName ));
        row.click();
        
        browser.sleep( 200 );
        
        var viewPage = element( by.css( '.view-volume-screen' ));
        var timelinePanel = viewPage.element( by.css( '.protection-policy' ));
        
        // sneaky way to scroll down
        var editIcon = timelinePanel.element( by.css( '.icon-edit' ));
        editIcon.click();
        
        // verify the value strings match what was passed in as the expected
        var labels = timelinePanel.all( by.css( '.value-label' ));
        
        labels.get(0).then( function( elem ){
            elem.getText().then( function( txt ){
                expect( txt ).toBe( values[0] );
            });
        });
        
        labels.get(1).then( function( elem ){
            elem.getText().then( function( txt ){
                expect( txt ).toBe( values[1] );              
            });
        });
        
        labels.get(2).then( function( elem ){
            elem.getText().then( function( txt ){
                expect( txt ).toBe( values[2] );            
            });
        });
        
        labels.get(3).then( function( elem ){
            elem.getText().then( function( txt ){
                expect( txt ).toBe( values[3] );
            });
        });        

    };
    
    it( 'should be able to create a volume with singular time settings', function(){
        login();
        
        createVolume( 'timelineVol', { type: 'object' }, { priority: 3, limit: 500, capacity: 90 },
            [
                {slider: 3, value: 1, unit: 3},
                {slider: 2, value: 30, unit: 2},
                {slider: 1, value: 1, unit: 1},
                {slider: 0, value: 1, unit: 0}
            ]);
        
        var row = element( by.cssContainingText( '.name', 'timelineVol' ));
        
        expect( row ).not.toBe( undefined );
        
        checkValues( 'timelineVol', ['1 day', '1 week', '30 days', '1 year'] );
        
    });
    
    it( 'should be able to create a volume with plural units', function(){
        
        createVolume( 'pluralTimelineVol', { type: 'object' }, undefined,
            [
                {slider: 3, value: 2, unit: 3},
                {slider: 2, value: 120, unit: 2},
                {slider: 1, value: 2, unit: 1},           
                {slider: 0, value: 2, unit: 0}            
            ]);
        
        var row = element( by.cssContainingText( '.name', 'pluralTimelineVol' ));
        
        expect( row ).not.toBe( undefined );
        
        checkValues( 'pluralTimelineVol', ['2 days', '2 weeks', '120 days', '2 years'] );        

    });
});