require( '../utils' );

describe( 'Testing volume creation permutations', function(){

    var STANDARD = 'Standard';
    var SPARSE = 'Sparse Coverage';
    var DENSE = 'Dense Coverage';
    var LEAST = 'Least Important';
    var MOST = 'Most Important';
    var CUSTOM = 'Custom';
    
    browser.get( '#/' );
    var createLink;
    var mainEl;
    var createEl;
    var newText;
    var viewEl;
    
    clean();
    
    var verifyVolume = function( name, media, qos, timeline ){
        
        viewEl.element( by.css( '.volume-name' )).getText().then( function( text ){
            expect( text ).toBe( name );
        });
        
        viewEl.element( by.css( '.media-policy' )).getText().then( function( text ){
            expect( text ).toBe( media );
        });
        
        viewEl.element( by.css( '.priority' )).getText().then( function( text ){
            expect( text ).toBe( qos.priority );
        });
        
        viewEl.element( by.css( '.sla' )).getText().then( function( text ){
            expect( text ).toBe( qos.sla );
        });
        
        viewEl.element( by.css( '.limit' )).getText().then( function( text ){
            expect( text ).toBe( qos.limit );
        });        
        
        viewEl.element( by.css( '.qos-preset' )).getText().then( function( text ){
            expect( text ).toBe( qos.preset );
        });       
    
        viewEl.all( by.css( '.predicate' )).then( function( elems ){

            elems[0].getText().then( function( text ){
                expect( text ).toBe( timeline.settings[0].predicate );
            });
            
            elems[1].getText().then( function( text ){
                expect( text ).toBe( timeline.settings[1].predicate );
            });
            
            elems[2].getText().then( function( text ){
                expect( text ).toBe( timeline.settings[2].predicate );
            });
            
            elems[3].getText().then( function( text ){
                expect( text ).toBe( timeline.settings[3].predicate );
            });
            
            elems[4].getText().then( function( text ){
                expect( text ).toBe( timeline.settings[4].predicate );
            });            
        });
        
        viewEl.all( by.css( '.retention-value' )).then( function( elems ){

            elems[0].getText().then( function( text ){
                expect( text ).toBe( timeline.settings[0].value );
            });
            
            elems[1].getText().then( function( text ){
                expect( text ).toBe( timeline.settings[1].value );
            });
            
            elems[2].getText().then( function( text ){
                expect( text ).toBe( timeline.settings[2].value );
            });
            
            elems[3].getText().then( function( text ){
                expect( text ).toBe( timeline.settings[3].value );
            });
            
            elems[4].getText().then( function( text ){
                expect( text ).toBe( timeline.settings[4].value );
            });            
        });    
        
        
    };
    
    var clickRow = function( name ){
        mainEl.all( by.css( '.volume-row td.name' )).each( function( elem ){
            elem.getText().then( function( text ){
                if ( text === name ){
                    elem.click();
                }
            });
        });
    };

    it( 'should not find any volumes in the table', function(){

        login();
        goto( 'volumes' );

        browser.sleep( 200 );

        createLink = element( by.css( 'a.new_volume') );
        createEl = $('.create-panel.volumes');
        createButton = element.all( by.buttonText( 'Create Volume' ) ).get(0);
        mainEl = element.all( by.css( '.slide-window-stack-slide' ) ).get(0);
        viewEl = element.all( by.css( '.slide-window-stack-slide' ) ).get(2);
        
        newText = createEl.element( by.css( '.volume-name-input') );
        newText = newText.element( by.tagName( 'input' ) );

        var volumeTable = $('tr');

        expect( volumeTable.length ).toBe( undefined );
    });

//    it( 'pressing create should show the create dialog and push the screen to the left', function(){
//
//        createLink.click();
//        browser.sleep( 200 );
//        expect( createEl.getAttribute( 'style' ) ).not.toContain( 'hidden' );
//        expect( mainEl.getAttribute( 'style' ) ).toContain( '-100%' );
//    });
//
//    it( 'should save with just a name', function(){
//        
//        newText.sendKeys( 'Test Volume' );
//        browser.sleep( 100 );
//        
//        createButton.click();
//
//        expect( mainEl.getAttribute( 'style' ) ).not.toContain( '-100%' );
//
//        element.all( by.repeater( 'volume in volumes' )).then( function( results ){
//            expect( results.length ).toBe( 1 );
//        });
//
//
//        element.all( by.css( '.volume-row .name' ) ).then( function( nameCols ){
//
//            nameCols.forEach( function( td ){
//                td.getText().then( function( txt ){
//                    expect( txt ).toBe( 'Test Volume' );
//                });
//            });
//        });
//
//        // check that the default priority was correct
//        element.all( by.css( '.volume-row .priority' ) ).then( function( priorityCols ){
//            priorityCols.forEach( function( td ){
//                td.getText().then( function( txt ){
//                    expect( txt ).toBe( '7' );
//                });
//            });
//        });
//    });
//    
//    it ( 'should go to the edit screen on press of the edit button', function(){
//        
//        clickRow( 'Test Volume' );
//        
//        // the screen should slide over
//        expect( mainEl.getAttribute( 'style' ) ).toContain( '-100%' );
//        expect( viewEl.getAttribute( 'style' ) ).toContain( '0%' );
//        
//        // title should be the volume title
//        viewEl.element( by.css( '.volume-name' )).getText().then( function( txt ){
//            expect( txt ).toBe( 'Test Volume' );
//        });
//        
//    });
//    
//    it ( 'should have default values for all other fields in the created volume', function(){
//        
//        // check the actual settings
//        verifyVolume( 
//            'Test Volume', 
//            'Flash Only',
//            { preset: STANDARD, priority: '7', sla: 'None', limit: 'Unlimited'},
//            { 
//                preset: STANDARD, 
//                settings: [
//                    { predicate: 'Kept', value: 'for 1 day' },
//                    { predicate: 'at 12am', value: 'for 1 week' },
//                    { predicate: 'Mondays', value: 'for 4 weeks' },
//                    { predicate: 'First day of the month', value: 'for 26 weeks' },
//                    { predicate: 'Januarys', value: 'for 5 years' }
//                ]
//            }
//        );
//        
//        // go back
//        var backLink = element( by.css( '.slide-window-stack-breadcrumb' ) ).click();
//    });
//    
//    it( 'should be able to create a volume with lowest presets', function(){
//        
//        var qos = { preset: 0 };
//        var timeline = { preset: 0 };
//        
//        var name = 'Dumb One';
//        
//        createVolume( name, undefined, qos, 'HYBRID_ONLY', timeline );
//
//        browser.sleep( 200 );
//        
//        clickRow( name );
//
//        browser.sleep( 300 );
//        
//        verifyVolume( 
//            name, 
//            'Hybrid',
//            { preset: LEAST, priority: '10', sla: 'None', limit: 'Unlimited'},
//            { 
//                preset: SPARSE, 
//                settings: [
//                    { predicate: 'Kept', value: 'for 1 day' },
//                    { predicate: 'at 12am', value: 'for 2 days' },
//                    { predicate: 'Mondays', value: 'for 1 week' },
//                    { predicate: 'First day of the month', value: 'for 4 weeks' },
//                    { predicate: 'Januarys', value: 'for 2 years' }
//                ]
//            }
//        );
//        
//        // go back
//        var backLink = element( by.css( '.slide-window-stack-breadcrumb' ) ).click();
//    });
//    
//    it( 'should be able to create a volume with the highest presets', function(){
//        var qos = { preset: 2 };
//        var timeline = { preset: 2 };
//        
//        var name = 'Awesome One';
//        
//        createVolume( name, undefined, qos, 'HDD_ONLY', timeline );
//
//        browser.sleep( 200 );
//        
//        clickRow( name );
//
//        browser.sleep( 200 );
//        
//        verifyVolume( 
//            name, 
//            'Disk Only',
//            { preset: MOST, priority: '1', sla: 'None', limit: 'Unlimited'},
//            { 
//                preset: DENSE, 
//                settings: [
//                    { predicate: 'Kept', value: 'for 2 days' },
//                    { predicate: 'at 12am', value: 'for 4 weeks' },
//                    { predicate: 'Mondays', value: 'for 30 weeks' },
//                    { predicate: 'First day of the month', value: 'for 2 years' },
//                    { predicate: 'Januarys', value: 'for 15 years' }
//                ]
//            }
//        ); 
//        
//        // go back
//        var backLink = element( by.css( '.slide-window-stack-breadcrumb' ) ).click();
//    });
    
    it( 'should be able to create a volume with custom settings', function(){
        
        var qos = {
            priority: 3,
            capacity: 60,
            limit: 1000
        };
        
        var timeline = [
            { slider: 0, value: 1, unit: 0 },
            { slider: 1, value: 3, unit: 0 },
            { slider: 2, value: 60, unit: 2 },
            { slider: 3, value: 1, unit: 3 },
            { slider: 4, value: 2, unit: 3 }
        ];
        
        var startTimes = {
            hours: 3,
            days: 3, 
            months: 0,
            years: 8
        };
        
        var data_type = {
            type: 'block',
            attributes: {
                size: 9
            }
        };
        
        var name = 'Custom Volume';
        
        createVolume( name, data_type, qos, 'SSD_ONLY', timeline, startTimes );
        
        browser.sleep( 200 );
        
        clickRow( name );

        browser.sleep( 300 );
        
        verifyVolume( 
            name, 
            'Flash Only',
            { preset: CUSTOM, priority: '3', sla: '60', limit: '1000'},
            { 
                preset: CUSTOM, 
                settings: [
                    { predicate: 'Kept', value: 'for 1 day' },
                    { predicate: 'at 3am', value: 'for 3 days' },
                    { predicate: 'Thursdays', value: 'for 60 days' },
                    { predicate: 'First day of the month', value: 'for 1 year' },
                    { predicate: 'Septembers', value: 'for 2 years' }
                ]
            }
        ); 
        
        // go back
        var backLink = element( by.css( '.slide-window-stack-breadcrumb' ) ).click();        
    });
//
//    it ( 'should be able to edit the priority of a volume', function(){
//
//        var editQosButton = viewEl.element( by.css( '.qos-panel .icon-edit' ));
//        editQosButton.click();
//        
//        browser.sleep( 200 );
//
//        // panel should now be editable
//        var slaDisplay = viewEl.element( by.css( '.volume-sla-edit-display' ) );
//        expect( slaDisplay.getAttribute( 'class' ) ).not.toContain( 'ng-hide' );
//
//        var prioritySliderSegments = viewEl.element( by.css( '.volume-priority-slider' ) ).all( by.css( '.segment' ) );
//        prioritySliderSegments.count().then( function( num ){
//            expect( num ).toBe( 9 );
//        });
//        
//        browser.actions().mouseMove( prioritySliderSegments.get( 7 ) ).click().perform();
//        
//        var doneButton = viewEl.element( by.css( '.save-qos-settings' ) );
//        doneButton.click();
//        browser.sleep( 210 );
//
//        var priorityDisplay = viewEl.element( by.css( '.volume-priority-display-only' ) );
//        priorityDisplay.getText().then( function( txt ){
//            expect( txt ).toContain( '8' );
//        });
//        
//        // done editing
//        element.all( by.css( '.slide-window-stack-breadcrumb' ) ).get( 0 ).click();
//        
//        browser.sleep( 210 );
//        
//        element.all( by.css( '.volume-row .priority' ) ).then( function( priorityCols ){
//            priorityCols.forEach( function( td ){
//                td.getText().then( function( txt ){
//                    expect( txt ).toBe( '8' );
//                });
//            });
//        });
//
//    });
//
//    it( 'should be able to delete a volume', function(){
//
//        deleteVolume( 0 );
//    });
//
//    it( 'should be able to cancel editing and show default values on next entry', function(){
//
//        createLink.click();
//        browser.sleep( 210 );
//
//        newText.sendKeys( 'This should go away' );
//
//        var cancelButton = element.all( by.css( 'button.cancel-creation' )).get( 0 );
//        cancelButton.click();
//        browser.sleep( 210 );
//
//        expect( mainEl.getAttribute( 'style' ) ).not.toContain( '-100%' );
//
//        createLink.click();
//        browser.sleep( 210 );
//
//        newText.getText().then( function( txt ){
//            expect( txt ).toBe( '' );
//        });
//
//        cancelButton.click();
//        browser.sleep( 210 );
//    });
//
//    it( 'should be able to create a volume and edit each portion', function(){
//
//        createLink.click();
//        browser.sleep( 200 );
//
//        // no text should be there now.
//        newText.getText().then( function( txt ){
//            expect( txt ).toBe( '' );
//        });
//        
//        newText.sendKeys( 'Complex Volume' );
//
//        // messing with the chosen data connector
//        var dcDisplay = createEl.element( by.css( '.data-connector-display' ));
//        expect( dcDisplay.getAttribute( 'class' ) ).not.toContain( 'ng-hide' );
//        
//        var editDcButton = createEl.element( by.css( '.edit-data-connector-button' ) );
//        editDcButton.click();
//        
//        expect( dcDisplay.getAttribute( 'class' ) ).toContain( 'ng-hide' );
//        
//        var typeMenu = createEl.element( by.css( '.data-connector-dropdown' ) );
//        var blockEl = typeMenu.all( by.tagName( 'li' ) ).first();
//        
//        typeMenu.click();
//        blockEl.click();
//        
//        var sizeSpinner = createEl.element( by.css( '.data-connector-size-spinner' )).element( by.tagName( 'input' ));
//        sizeSpinner.clear();
//        sizeSpinner.sendKeys( '1' );
//        
//        var doneName = createEl.element( by.css( '.save-name-type-data' ));
//        doneName.click();
//        
//        dcDisplay.getText().then( function( txt ){
//            expect( txt ).toBe( 'Block' );
//        });
//
//        // changing the QoS
//        var editQosButton = createEl.element( by.css( '.qos-panel .icon-edit' ));
//        editQosButton.click();
//        
//        browser.sleep( 200 );
//
//        // panel should now be editable
//        var slaEditDisplay = createEl.element( by.css( '.volume-sla-edit-display' ) );
//        expect( slaEditDisplay.getAttribute( 'class' ) ).not.toContain( 'ng-hide' );
//        
//        var slaTableDisplay = createEl.element( by.css( '.volume-display-only' ) );
//        expect( slaTableDisplay.getAttribute( 'class' ) ).toContain( 'ng-hide' );
//        
//        browser.actions().mouseMove( createEl.element( by.css( '.volume-priority-slider' ) ).all( by.css( '.segment' )).get( 1 ) ).click().perform();
//        
//        browser.actions().mouseMove( createEl.element( by.css( '.volume-iops-slider' ) ).all( by.css( '.segment' )).get( 8 ) ).click().perform();
//        
//        browser.actions().mouseMove( createEl.element( by.css( '.volume-limit-slider' ) ).all( by.css( '.segment' )).get( 7 ) ).click().perform();
//        
//        var doneButton = createEl.element( by.css( '.save-qos-settings' ) );
//        doneButton.click();
//
//        expect( slaTableDisplay.getAttribute( 'class' ) ).not.toContain( 'ng-hide' );
//        expect( slaEditDisplay.getAttribute( 'class' ) ).toContain( 'ng-hide' );
//        
//        var priorityDisplay = createEl.element( by.css( '.volume-priority-display-only' ) );
//        var slaDisplay = createEl.element( by.css( '.volume-sla-display' ) );
//        var limitDisplay = createEl.element( by.css( '.volume-limit-display' ) );
//        
//        priorityDisplay.getText().then( function( txt ){
//            expect( txt ).toBe( '2' );
//        });
//        
//        slaDisplay.getText().then( function( txt ){
//            expect( txt) .toBe( '90%' );
//        });
//        
//        limitDisplay.getText().then( function( txt ){
//            expect( txt ).toContain( '2000' );
//        });
//
//        element.all( by.buttonText( 'Create Volume' )).get( 0 ).click();
//        browser.sleep( 300 );
//
//        deleteVolume( 0 );
//        
//        element.all( by.css( '.volume-row .priority' ) ).then( function( priorityCols ){
//            priorityCols.forEach( function( td ){
//                td.getText().then( function( txt ){
//                    expect( txt ).toBe( '2' );
//                });
//            });
//        });
//        
//        logout();
//    });

});
