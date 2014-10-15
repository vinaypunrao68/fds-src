require( '../utils' );

describe( 'Testing volume creation permutations', function(){

    browser.get( '#/' );
    var createLink;
    var mainEl;
    var createEl;
    var newText;


//    browser.addMockModule( 'qos', mockSnap );

    it( 'should not find any volumes in the table', function(){

        login();
        goto( 'volumes' );

        browser.sleep( 200 );

        createLink = element( by.css( 'a.new_volume') );
        createEl = $('.create-panel.volumes');
        createButton = element.all( by.buttonText( 'Create Volume' ) ).get(0);
        mainEl = element.all( by.css( '.slide-window-stack-slide' ) ).get(0);
        newText = element( by.model( 'name') );

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
        
        var edit = element( by.css( '.button-group span.fui-new' ) );
        edit.click();

        browser.sleep( 210 );
        
        // the screen should slide over
        expect( mainEl.getAttribute( 'style' ) ).toContain( '-100%' );
        
        // title should say "edit"
        var editTitle = element( by.css( '.create-volume-edit-title' ) );
        expect( editTitle.getAttribute( 'class' ) ).not.toContain( 'ng-hide' );
        
        // name field shouldn't be editable
        var input = element( by.css( 'input.volume-name' ));
        expect( input.isEnabled() ).toBe( false );
        
        // make sure the clone section isn't present
        var cloneBlock = element( by.css( '.clone-volume-selector' ));
        expect( cloneBlock.getAttribute( 'class' )).toContain( 'ng-hide' );
        
    });

    it ( 'should be able to edit the priority of a volume', function(){

        var editQosButton = element( by.css( '.qos-panel .fui-new' ));
        editQosButton.click();
        
        browser.sleep( 200 );

        // panel should now be editable
        var priorityDisplay = element( by.css( '.volume-priority-display-only' ) );
        expect( priorityDisplay.getAttribute( 'class' ) ).toContain( 'ng-hide' );
        
        var prioritySpinner = element( by.css( '.volume-priority-spinner' ) ).element( by.tagName( 'input' ) );
        prioritySpinner.clear();
        prioritySpinner.sendKeys( '8' );
        
        var doneButton = element( by.css( '.save-qos-settings' ) );
        doneButton.click();
        browser.sleep( 210 );

        priorityDisplay.getText().then( function( txt ){
            expect( txt ).toBe( '8' );
        });
        
        // save the change
        var editButton = element.all( by.buttonText( 'Edit Volume' ) ).get( 0 );
        editButton.click();
        
        browser.sleep( 210 );
        
        element.all( by.css( '.volume-row .priority' ) ).then( function( priorityCols ){
            priorityCols.forEach( function( td ){
                td.getText().then( function( txt ){
                    expect( txt ).toBe( '8' );
                });
            });
        });

    });

//    it( 'should be able to delete a volume', function(){
//
//        var delButton = element( by.css( 'span.fui-cross' ) );
//        delButton.click();
//        browser.sleep( 100 );
//
//        var alert = browser.switchTo().alert();
//        alert.accept();
//
//        browser.sleep( 200 );
//
//        element.all( by.css( 'volume-row' )).then( function( rows ){
//            expect( rows.length ).toBe( 0 );
//        });
//    });

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

        newText.sendKeys( 'Complex Volume' );

        // messing with the chosen data connector
        var dcDisplay = element( by.css( '.data-connector-display' ));
        expect( dcDisplay.getAttribute( 'class' ) ).not.toContain( 'ng-hide' );
        
        var editDcButton = element( by.css( '.edit-data-connector-button' ) );
        editDcButton.click();
        
        expect( dcDisplay.getAttribute( 'class' ) ).toContain( 'ng-hide' );
        
        var typeMenu = element( by.css( '.data-connector-dropdown' ) );
        var blockEl = typeMenu.all( by.tagName( 'li' ) ).first();
        
        typeMenu.click();
        blockEl.click();
        
        var sizeSpinner = element( by.css( '.data-connector-size-spinner' )).element( by.tagName( 'input' ));
        sizeSpinner.clear();
        sizeSpinner.sendKeys( '1' );
        
        var doneName = element( by.css( '.save-name-type-data' ));
        doneName.click();
        
        dcDisplay.getText().then( function( txt ){
            expect( txt ).toBe( 'Block' );
        });

        // changing the QoS
        var editQosButton = element( by.css( '.qos-panel .fui-new' ));
        editQosButton.click();
        
        browser.sleep( 200 );

        // panel should now be editable
        var priorityDisplay = element( by.css( '.volume-priority-display-only' ) );
        expect( priorityDisplay.getAttribute( 'class' ) ).toContain( 'ng-hide' );
        
        var slaDisplay = element( by.css( '.volume-sla-display' ) );
        expect( slaDisplay.getAttribute( 'class' ) ).toContain( 'ng-hide' );
        
        var limitDisplay = element( by.css( '.volume-limit-display' ) );
        expect( limitDisplay.getAttribute( 'class' ) ).toContain( 'ng-hide' );
        
        var prioritySpinner = element( by.css( '.volume-priority-spinner' ) ).element( by.tagName( 'input' ) );
        prioritySpinner.clear();
        prioritySpinner.sendKeys( '2' );
        
        var slaSpinner = element( by.css( '.volume-sla-spinner' )).element( by.tagName( 'input' ));
        slaSpinner.clear();
        slaSpinner.sendKeys( '90' );
        
        var limitSpinner = element( by.css( '.volume-limit-spinner' )).element( by.tagName( 'input' ));
        limitSpinner.clear();
        limitSpinner.sendKeys( '2000' );
        
        var doneButton = element( by.css( '.save-qos-settings' ) );
        doneButton.click();

        expect( priorityDisplay.getAttribute( 'class' ) ).not.toContain( 'ng-hide' );
        expect( slaDisplay.getAttribute( 'class' ) ).not.toContain( 'ng-hide' );
        expect( limitDisplay.getAttribute( 'class' ) ).not.toContain( 'ng-hide' );
        
        priorityDisplay.getText().then( function( txt ){
            expect( txt ).toBe( '2' );
        });
        
        slaDisplay.getText().then( function( txt ){
            expect( txt) .toBe( '90%' );
        });
        
        limitDisplay.getText().then( function( txt ){
            expect( txt ).toBe( '2000' );
        });

        element.all( by.buttonText( 'Create Volume' )).get( 0 ).click();
        browser.sleep( 300 );

//        element.all( by.css( '.volume-row .priority' ) ).then( function( priorityCols ){
//            priorityCols.forEach( function( td ){
//                td.getText().then( function( txt ){
//                    expect( txt ).toBe( '2' );
//                });
//            });
//        });

//        element( by.css( 'span.fui-cross' )).click();
//        browser.sleep( 200 );
//        browser.switchTo().alert().accept();
    });

});
