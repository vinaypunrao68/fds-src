require( '../utils' );

describe( 'Testing the timeline settings', function(){
    
//    var checkValues = function( volumeName, values ){
//        
//        var row = element( by.cssContainingText( '.name', volumeName ));
//        row.click();
//        
//        browser.sleep( 200 );
//        
//        var viewPage = element( by.css( '.view-volume-screen' ));
//        var timelinePanel = viewPage.element( by.css( '.protection-policy' ));
//        
//        // sneaky way to scroll down
////        var editIcon = timelinePanel.element( by.css( '.icon-edit' ));
////        editIcon.click();
//        
//        // verify the value strings match what was passed in as the expected
//        var labels = timelinePanel.all( by.css( '.value-label' ));
//        
//        labels.get(0).then( function( elem ){
//            elem.getText().then( function( txt ){
//                console.log( '0: ' + txt );
//                expect( txt ).toBe( values[0] );
//            });
//        });
//        
//        labels.get(1).then( function( elem ){
//            elem.getText().then( function( txt ){
//                console.log( '1: ' + txt );
//                expect( txt ).toBe( values[1] );              
//            });
//        });
//        
//        labels.get(2).then( function( elem ){
//            elem.getText().then( function( txt ){
//                console.log( '2: ' + txt );
//                expect( txt ).toBe( values[2] );            
//            });
//        });
//        
//        labels.get(3).then( function( elem ){
//            elem.getText().then( function( txt ){
//                console.log( '3: ' + txt );
//                expect( txt ).toBe( values[3] );
//            });
//        });    
//        
//        labels.get(4).then( function( elem ){
//            elem.getText().then( function( txt ){
//                console.log( '4: ' + txt );
//                expect( txt ).toBe( values[4] );
//            });
//        });          
//
//    };
//    
//    it( 'should be able to create a volume with the defaults', function(){
//        login();
//        
//        var name = 'defaultTimeline';
//        
//        createVolume( name );
//        
//        var row = element( by.cssContainingText( '.name', name ));
//        
//        expect( row ).not.toBe( undefined );
//        
//        checkValues( name, ['1 day', '1 week', '2 weeks', '60 days', '1 year'] );
//        
//        element.all( by.css( '.slide-window-stack-breadcrumb' )).get( 0 ).click();
//        browser.sleep( 200 );        
//    });
//
//    it( 'should be able to create a volume with singular time settings', function(){
//        
//        createVolume( 'timelineVol', { type: 'object' }, { priority: 3, limit: 500, capacity: 90 },
//            [
//                {slider: 4, value: 16, unit: 4 },
//                {slider: 3, value: 1, unit: 3},
//                {slider: 2, value: 30, unit: 2},
//                {slider: 1, value: 1, unit: 1},
//                {slider: 0, value: 1, unit: 0}
//            ]);
//        
//        var row = element( by.cssContainingText( '.name', 'timelineVol' ));
//        
//        expect( row ).not.toBe( undefined );
//        
//        checkValues( 'timelineVol', ['1 day', '1 week', '30 days', '1 year', '\u221E'] );
//        
//        element.all( by.css( '.slide-window-stack-breadcrumb' )).get( 0 ).click();
//        browser.sleep( 200 );
//    });
//    
////    it( 'should be able to create a volume with plural units', function(){
////        
////        createVolume( 'pluralTimelineVol', { type: 'object' }, { priority: 3, limit: 500, capacity: 90 },
////            [
////                {slider: 4, value: 3, unit: 3 },
////                {slider: 3, value: 2, unit: 3},
////                {slider: 2, value: 2, unit: 1},
////                {slider: 1, value: 2, unit: 1},           
////                {slider: 0, value: 2, unit: 0}            
////            ]);
////        
////        var row = element( by.cssContainingText( '.name', 'pluralTimelineVol' ));
////        
////        expect( row ).not.toBe( undefined );
////        
////        checkValues( 'pluralTimelineVol', ['2 days', '2 weeks', '120 days', '2 years', '3 years'] );   
////        
////        element.all( by.css( '.slide-window-stack-breadcrumb' )).get( 0 ).click();
////        browser.sleep( 200 );
////    });
//    
//    it( 'should be able to create a volume with different values for the time combos', function(){
//        
//        var name = 'diffStartTimes';
//        
//        createVolume( name, undefined, undefined, undefined, {hours: 2, days: 2, months: 2, years: 2});
//
//        var row = element( by.cssContainingText( '.name', name ));
//        expect( row ).not.toBe( undefined );
//        row.click();
//        browser.sleep( 200 );
//        
//        var pageEl = $('.view-volume-screen');
//        var timelinePanel = pageEl.element( by.css( '.protection-policy' ));
//
//        var hourChoice = timelinePanel.element( by.css( '.hour-choice' )).getText().then( function( txt ){
//            expect( txt ).toBe( '2:00a.m.' );
//        });
//        
//        var dayChoice = timelinePanel.element( by.css( '.day-choice' )).getText().then( function( txt ){
//            expect( txt ).toBe( 'on Wednesdays' );
//        });
//
//        var monthChoice = timelinePanel.element( by.css( '.month-choice' )).getText().then( function( txt ){
//            expect( txt ).toBe( 'First week of the month' );
//        });
//
//        var yearChoice = timelinePanel.element( by.css( '.year-choice' )).getText().then( function( txt ){
//            expect( txt ).toBe( 'March' );
//        });
//
//        element.all( by.css( '.slide-window-stack-breadcrumb' )).get( 0 ).click();
//        browser.sleep( 200 );
//    });
//    
//    it( 'should be able to edit a policy', function(){
//        
//        var row = element( by.cssContainingText( '.name', 'diffStartTimes' ));
//        expect( row ).not.toBe( undefined );
//        row.click();
//        browser.sleep( 200 );
//        
//        var pageEl = $('.view-volume-screen');
//        var timelinePanel = pageEl.element( by.css( '.protection-policy' ));
////        timelinePanel.element( by.css( '.icon-edit' )).click();
//
//        setTimelineTimes( pageEl, [
//            {slider: 4, value: 3, unit: 3 },
//            {slider: 3, value: 2, unit: 3},
//            {slider: 2, value: 120, unit: 2},
//            {slider: 1, value: 2, unit: 1},           
//            {slider: 0, value: 2, unit: 0}             
//        ]);
//        
//        element.all( by.css( '.slide-window-stack-breadcrumb' )).get( 0 ).click();
//        browser.sleep( 200 );
//    
////        checkValues( 'diffStartTimes', ['2 days', '2 weeks', '120 days', '2 years', '3 years'] );        
//    });
});