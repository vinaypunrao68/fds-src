require( '../utils' );

describe( 'Testing volume creation permutations', function(){

    browser.get( '#/' );
    var createLink;
    var mainEl;
    var createEl;
    var newText;
    var viewEl;
    
    clean();

    it( 'should not find any volumes in the table', function(){

        login();
        goto( 'volumes' );

        browser.sleep( 200 );

        createLink = element( by.css( 'a.new_volume') );
        createEl = $('.create-panel.volumes');
        createButton = element.all( by.buttonText( 'Create Volume' ) ).get(0);
        mainEl = element.all( by.css( '.slide-window-stack-slide' ) ).get(0);
        viewEl = element.all( by.css( '.slide-window-stack-slide' ) ).get(2);
        
        newText = element( by.css( '.volume-name-input') );
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
        browser.sleep( 100 );
        
        createButton.click();

        expect( mainEl.getAttribute( 'style' ) ).not.toContain( '-100%' );

        element.all( by.repeater( 'volume in volumes' )).then( function( results ){
            expect( results.length ).toBe( 1 );
        });


        element.all( by.css( '.volume-row .name' ) ).then( function( nameCols ){

            nameCols.forEach( function( td ){
                td.getText().then( function( txt ){
                    expect( txt ).toBe( 'Test Volume' );
                });
            });
        });

        // check that the default priority was correct
        element.all( by.css( '.volume-row .priority' ) ).then( function( priorityCols ){
            priorityCols.forEach( function( td ){
                td.getText().then( function( txt ){
                    expect( txt ).toBe( '10' );
                });
            });
        });
    });
    
    it ( 'should go to the edit screen on press of the edit button', function(){
        
        var firstRow = element.all( by.css( '.volume-row' ) ).get( 0 );
        firstRow.click();

        browser.sleep( 210 );
        
        // the screen should slide over
        expect( mainEl.getAttribute( 'style' ) ).toContain( '-100%' );
        expect( viewEl.getAttribute( 'style' ) ).toContain( '0%' );
        
        // title should be the volume title
        viewEl.element( by.css( '.volume-name' )).getText().then( function( txt ){
            expect( txt ).toBe( 'Test Volume' );
        });
        
    });

    it ( 'should be able to edit the priority of a volume', function(){

        var editQosButton = viewEl.element( by.css( '.qos-panel .icon-edit' ));
        editQosButton.click();
        
        browser.sleep( 200 );

        // panel should now be editable
        var slaDisplay = viewEl.element( by.css( '.volume-sla-edit-display' ) );
        expect( slaDisplay.getAttribute( 'class' ) ).not.toContain( 'ng-hide' );

        var prioritySliderSegments = viewEl.element( by.css( '.volume-priority-slider' ) ).all( by.css( '.segment' ) );
        prioritySliderSegments.count().then( function( num ){
            expect( num ).toBe( 9 );
        });
        
        browser.actions().mouseMove( prioritySliderSegments.get( 7 ) ).click().perform();
        
        var doneButton = viewEl.element( by.css( '.save-qos-settings' ) );
        doneButton.click();
        browser.sleep( 210 );

        var priorityDisplay = viewEl.element( by.css( '.volume-priority-display-only' ) );
        priorityDisplay.getText().then( function( txt ){
            expect( txt ).toContain( '8' );
        });
        
        // done editing
        element.all( by.css( '.slide-window-stack-breadcrumb' ) ).get( 0 ).click();
        
        browser.sleep( 210 );
        
        element.all( by.css( '.volume-row .priority' ) ).then( function( priorityCols ){
            priorityCols.forEach( function( td ){
                td.getText().then( function( txt ){
                    expect( txt ).toBe( '8' );
                });
            });
        });

    });

    it( 'should be able to delete a volume', function(){

        deleteVolume( 0 );
    });

    it( 'should be able to cancel editing and show default values on next entry', function(){

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

    it( 'should be able to create a volume and edit each portion', function(){

        createLink.click();
        browser.sleep( 200 );

        // no text should be there now.
        newText.getText().then( function( txt ){
            expect( txt ).toBe( '' );
        });
        
        newText.sendKeys( 'Complex Volume' );

        // messing with the chosen data connector
        var dcDisplay = createEl.element( by.css( '.data-connector-display' ));
        expect( dcDisplay.getAttribute( 'class' ) ).not.toContain( 'ng-hide' );
        
        var editDcButton = createEl.element( by.css( '.edit-data-connector-button' ) );
        editDcButton.click();
        
        expect( dcDisplay.getAttribute( 'class' ) ).toContain( 'ng-hide' );
        
        var typeMenu = createEl.element( by.css( '.data-connector-dropdown' ) );
        var blockEl = typeMenu.all( by.tagName( 'li' ) ).first();
        
        typeMenu.click();
        blockEl.click();
        
        var sizeSpinner = createEl.element( by.css( '.data-connector-size-spinner' )).element( by.tagName( 'input' ));
        sizeSpinner.clear();
        sizeSpinner.sendKeys( '1' );
        
        var doneName = createEl.element( by.css( '.save-name-type-data' ));
        doneName.click();
        
        dcDisplay.getText().then( function( txt ){
            expect( txt ).toBe( 'Block' );
        });

        // changing the QoS
        var editQosButton = createEl.element( by.css( '.qos-panel .icon-edit' ));
        editQosButton.click();
        
        browser.sleep( 200 );

        // panel should now be editable
        var slaEditDisplay = createEl.element( by.css( '.volume-sla-edit-display' ) );
        expect( slaEditDisplay.getAttribute( 'class' ) ).not.toContain( 'ng-hide' );
        
        var slaTableDisplay = createEl.element( by.css( '.volume-display-only' ) );
        expect( slaTableDisplay.getAttribute( 'class' ) ).toContain( 'ng-hide' );
        
        browser.actions().mouseMove( createEl.element( by.css( '.volume-priority-slider' ) ).all( by.css( '.segment' )).get( 1 ) ).click().perform();
        
        browser.actions().mouseMove( createEl.element( by.css( '.volume-iops-slider' ) ).all( by.css( '.segment' )).get( 8 ) ).click().perform();
        
        browser.actions().mouseMove( createEl.element( by.css( '.volume-limit-slider' ) ).all( by.css( '.segment' )).get( 7 ) ).click().perform();
        
        var doneButton = createEl.element( by.css( '.save-qos-settings' ) );
        doneButton.click();

        expect( slaTableDisplay.getAttribute( 'class' ) ).not.toContain( 'ng-hide' );
        expect( slaEditDisplay.getAttribute( 'class' ) ).toContain( 'ng-hide' );
        
        var priorityDisplay = createEl.element( by.css( '.volume-priority-display-only' ) );
        var slaDisplay = createEl.element( by.css( '.volume-sla-display' ) );
        var limitDisplay = createEl.element( by.css( '.volume-limit-display' ) );
        
        priorityDisplay.getText().then( function( txt ){
            expect( txt ).toBe( '2' );
        });
        
        slaDisplay.getText().then( function( txt ){
            expect( txt) .toBe( '90%' );
        });
        
        limitDisplay.getText().then( function( txt ){
            expect( txt ).toContain( '2000' );
        });

        element.all( by.buttonText( 'Create Volume' )).get( 0 ).click();
        browser.sleep( 300 );

        deleteVolume( 0 );
        
        element.all( by.css( '.volume-row .priority' ) ).then( function( priorityCols ){
            priorityCols.forEach( function( td ){
                td.getText().then( function( txt ){
                    expect( txt ).toBe( '2' );
                });
            });
        });
        
        logout();
    });

});
