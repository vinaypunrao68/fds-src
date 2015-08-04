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
    var createButton;
    
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
            expect( text ).toBe( qos.iopsMin );
        });
        
        viewEl.element( by.css( '.limit' )).getText().then( function( text ){
            expect( text ).toBe( qos.iopsMax );
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
        createButton = element.all( by.css( '.save-volume' ) ).get(0);
        mainEl = element.all( by.css( '.slide-window-stack-slide' ) ).get(0);
        viewEl = element.all( by.css( '.slide-window-stack-slide' ) ).get(2);
        
        newText = createEl.element( by.css( '.volume-name-input') );
        newText = newText.element( by.tagName( 'input' ) );

        var volumeTable = $('tr');

        expect( volumeTable.length ).toBe( undefined );
    });

    it( 'pressing create should show the create dialog and push the screen to the left', function(){

        createLink.click();
        browser.sleep( 200 );
        expect( createEl.getAttribute( 'style' ) ).not.toContain( 'hidden' );
        expect( mainEl.getAttribute( 'style' ) ).toContain( '-100%' );
    });

    it( 'should save with just a name', function(){
        
        newText.sendKeys( 'Test Volume' );
        browser.sleep( 200 );
        
        createButton.click();

        expect( mainEl.getAttribute( 'style' ) ).not.toContain( '-100%' );

        var testRow = element( by.cssContainingText( '.volume-row', 'Test Volume' ));

        testRow.all( by.css( '.name' ) ).then( function( nameCols ){

            nameCols.forEach( function( td ){
                td.getText().then( function( txt ){
                    expect( txt ).toBe( 'Test Volume' );
                });
            });
        });

        // check that the default priority was correct
        testRow.all( by.css( '.priority' ) ).then( function( priorityCols ){
            priorityCols.forEach( function( td ){
                td.getText().then( function( txt ){
                    expect( txt ).toBe( '7' );
                });
            });
        });
    });
    
    it ( 'should go to the view screen on clicking the row', function(){
        
        clickRow( 'Test Volume' );
        
        browser.sleep( 200 );
        
        // the screen should slide over
        expect( mainEl.getAttribute( 'style' ) ).toContain( '-100%' );
        expect( viewEl.getAttribute( 'style' ) ).toContain( '0%' );
        
        // title should be the volume title
        viewEl.element( by.css( '.volume-name' )).getText().then( function( txt ){
            expect( txt ).toBe( 'Test Volume' );
        });
        
    });
    
    it ( 'should have default values for all other fields in the created volume', function(){
        
        // check the actual settings
        verifyVolume( 
            'Test Volume', 
            'Flash Only',
            { preset: STANDARD, priority: '7', iopsMin: 'None', iopsMax: 'Unlimited'},
            { 
                preset: STANDARD, 
                settings: [
                    { predicate: 'Kept', value: 'for 1 day' },
                    { predicate: 'at 12am', value: 'for 1 week' },
                    { predicate: 'Mondays', value: 'for 90 days' },
                    { predicate: 'First day of the month', value: 'for 180 days' },
                    { predicate: 'January', value: 'for 5 years' }
                ]
            }
        );
        
        // go back
        var backLink = element( by.css( '.slide-window-stack-breadcrumb' ) ).click();
    });
    
    it( 'should be able to create a volume with lowest presets', function(){
        
        var qos = { preset: 0 };
        var timeline = { preset: 0 };
        
        var name = 'Dumb One';
        
        createVolume( name, undefined, qos, 'HYBRID_ONLY', timeline );

        browser.sleep( 200 );
        
        clickRow( name );

        browser.sleep( 300 );
        
        verifyVolume( 
            name, 
            'Hybrid',
            { preset: LEAST, priority: '10', iopsMin: 'None', iopsMax: 'Unlimited'},
            { 
                preset: SPARSE, 
                settings: [
                    { predicate: 'Kept', value: 'for 1 day' },
                    { predicate: 'at 12am', value: 'for 2 days' },
                    { predicate: 'Mondays', value: 'for 1 week' },
                    { predicate: 'First day of the month', value: 'for 90 days' },
                    { predicate: 'January', value: 'for 2 years' }
                ]
            }
        );
        
        // go back
        var backLink = element( by.css( '.slide-window-stack-breadcrumb' ) ).click();
    });
    
    it( 'should be able to create a volume with the highest presets', function(){
        var qos = { preset: 2 };
        var timeline = { preset: 2 };
        
        var name = 'Awesome One';
        
        createVolume( name, undefined, qos, 'HDD_ONLY', timeline );

        browser.sleep( 200 );
        
        clickRow( name );

        browser.sleep( 200 );
        
        verifyVolume( 
            name, 
            'Disk Only',
            { preset: MOST, priority: '1', iopsMin: 'None', iopsMax: 'Unlimited'},
            { 
                preset: DENSE, 
                settings: [
                    { predicate: 'Kept', value: 'for 2 days' },
                    { predicate: 'at 12am', value: 'for 30 days' },
                    { predicate: 'Mondays', value: 'for 210 days' },
                    { predicate: 'First day of the month', value: 'for 2 years' },
                    { predicate: 'January', value: 'for 15 years' }
                ]
            }
        ); 
        
        // go back
        var backLink = element( by.css( '.slide-window-stack-breadcrumb' ) ).click();
    });
    
    it( 'should be able to create a volume with custom settings', function(){
        
        var qos = {
            priority: 3,
            iopsMin: 500,
            iopsMax: 1000
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
            type: 'BLOCK',
            capacity: {
                value: 10,
                unit: 'GB'
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
            { preset: CUSTOM, priority: '3', iopsMin: '500', iopsMax: '1000'},
            { 
                preset: CUSTOM, 
                settings: [
                    { predicate: 'Kept', value: 'for 1 day' },
                    { predicate: 'at 3am', value: 'for 3 days' },
                    { predicate: 'Thursdays', value: 'for 60 days' },
                    { predicate: 'First day of the month', value: 'for 1 year' },
                    { predicate: 'September', value: 'for 2 years' }
                ]
            }
        ); 
        
        // go back
        var backLink = element( by.css( '.slide-window-stack-breadcrumb' ) ).click();        
    });
    
    it ( 'should be able to switch presets for a volume', function(){
        
        browser.sleep( 220 );
        
        var qos = { preset: 2 };
        var timeline = { preset: 2 };
        
        var name = 'Dumb One';

        editVolume( name, undefined, qos, 'HDD_ONLY', timeline );
        
        verifyVolume( 
            name, 
            'Hybrid',
            { preset: MOST, priority: '1', iopsMin: 'None', iopsMax: 'Unlimited'},
            { 
                preset: DENSE, 
                settings: [
                    { predicate: 'Kept', value: 'for 2 days' },
                    { predicate: 'at 12am', value: 'for 30 days' },
                    { predicate: 'Mondays', value: 'for 210 days' },
                    { predicate: 'First day of the month', value: 'for 2 years' },
                    { predicate: 'January', value: 'for 15 years' }
                ]
            }
        ); 
        
        // go back
        var backLink = element( by.css( '.slide-window-stack-breadcrumb' ) ).click();    
    });

    it( 'should be able to cancel editing and show default values on next entry', function(){

        browser.sleep( 220 );
        createLink.click();
        browser.sleep( 210 );

        newText.sendKeys( 'This should go away' );

        var cancelButton = element.all( by.css( 'button.cancel-creation' )).get( 0 );
        cancelButton.click();
        browser.sleep( 210 );

        expect( mainEl.getAttribute( 'style' ) ).not.toContain( '-100%' );

        createLink.click();
        browser.sleep( 210 );

        newText.getText().then( function( txt ){
            expect( txt ).toBe( '' );
        });

        cancelButton.click();
        browser.sleep( 210 );
        
    });
    
    it( 'should be able to delete a volume', function(){
        
        deleteVolume( "Dumb One" );
        browser.sleep( 300 );
        
        var rows = mainEl.all( by.css( '.volume-row' ) ).count().then( function( num ){
            expect( num ).toBe( 3 );
        });
        
        logout();
    });

});
