require( '../utils' );

describe( 'Testing volume creation permutations', function(){

//    browser.get( '#/' );
    var snapshotPolicyEdit;
    var saveButton;
    var snapVolume;
    var createEl;
    var viewEl;
    
    it ( 'should be able to create a volume with a snapshot policy', function(){
        
        // create a volume with a daily and monthly snapshot policy
        login();
        goto( 'volumes' );
        browser.sleep( 220 );
        element( by.css( '.new_volume' ) ).click();
        browser.sleep( 200 );
        
        createEl = $('.create-panel.volumes');
        viewEl = element.all( by.css( '.slide-window-stack-slide' ) ).get(2);
        
        snapshotPolicyEdit = createEl.element( by.css( '.edit-snapshot-policies' ) );
        snapshotPolicyEdit.click();
        
        // make sure the displays have been toggled
        createEl.all( by.css( '.policy-value-display' ) ).each( function( el ){
            expect( el.getAttribute( 'class' ) ).toContain( 'ng-hide' );
        });
        
        // use daily for 2 days and monthly for 3 weeks
        var checks = createEl.all( by.css( '.tristatecheck' ));
        checks.get( 0 ).element( by.css( '.unchecked' )).click();
        checks.get( 2 ).element( by.css( '.unchecked' )).click();
        
        var spinners = createEl.all( by.css( '.retention-spinner' ) );
        var combos = createEl.all( by.css( '.retention-dropdown' ) );
        spinners.get( 0 ).element( by.tagName( 'input' )).clear().sendKeys( '2' );
        combos.get( 0 ).click();
        combos.get( 0 ).all( by.tagName( 'li' )).get( 1 ).click();
        
        spinners.get( 2 ).element( by.tagName( 'input' )).clear().sendKeys( '3' );
        combos.get( 2 ).click();
        combos.get( 2 ).all( by.tagName( 'li' )).get( 2 ).click();
        
        saveButton = createEl.element( by.css( '.save-snapshot-policies' ));
        saveButton.click();
        
        // give it a name
        createEl.element( by.css( '.volume-name' )).clear().sendKeys( 'SnapshotVolume' );
        element.all( by.buttonText( 'Create Volume' )).get( 0 ).click();
        
    });
    
    it ( 'should be able to be edited', function(){
        
        // find our volume
        snapVolume = element( by.cssContainingText( '.volume-row', 'SnapshotVolume' ));
        snapVolume.click();
        
        var checks = viewEl.all( by.css( '.tristatecheck' ));
        var spinners = viewEl.all( by.css( '.retention-spinner' ) );
        var combos = viewEl.all( by.css( '.retention-dropdown' ) );
        
        // verify the snapshot policy is there
        expect( checks.get( 0 ).element( by.css( '.checked' )).getAttribute( 'class' ) ).not.toContain( 'ng-hide' );
        expect( checks.get( 2 ).element( by.css( '.checked' )).getAttribute( 'class' ) ).not.toContain( 'ng-hide' );
        
        var texts = viewEl.all( by.css( '.policy-value-display' ) );
        texts.get( 0 ).getText().then( function( txt ){
            expect( txt ).toContain( '2 days' );
        });
        
        texts.get( 2 ).getText().then( function( txt ){
            expect( txt ).toContain( '3 weeks' );
        });
        
        // edit it
        snapshotPolicyEdit = viewEl.element( by.css( '.edit-snapshot-policies' ) );
        snapshotPolicyEdit.click();
        checks.get( 0 ).element( by.css( '.checked' ) ).click();
        checks.get( 2 ).element( by.css( '.checked' ) ).click();
        checks.get( 1 ).element( by.css( '.unchecked' ) ).click();
        
        combos.get( 1 ).click();
        combos.get( 1 ).all( by.tagName( 'li' ) ).get( 0 ).click();
        
        saveButton = viewEl.element( by.css( '.save-snapshot-policies' ));
        saveButton.click();
        
        // done editing
        element.all( by.css( '.slide-window-stack-breadcrumb' ) ).get( 0 ).click();
        
        browser.sleep( 200 );
        
        // click "new" and make sure the policy isn't there
        element( by.css( '.new_volume' ) ).click();
        
        checks = createEl.all( by.css( '.tristatecheck' ));
        expect( checks.get( 0 ).element( by.css( '.checked' ) ).getAttribute( 'class' ) ).toContain( 'ng-hide' );
        expect( checks.get( 1 ).element( by.css( '.checked' ) ).getAttribute( 'class' ) ).toContain( 'ng-hide' );
        expect( checks.get( 2 ).element( by.css( '.checked' ) ).getAttribute( 'class' ) ).toContain( 'ng-hide' );
        
        element.all( by.buttonText( 'Cancel' ) ).get( 0 ).click();
        
        browser.sleep( 210 );
        
    });
    
    it ( 'should be able to take a snapshot and see it in the list', function(){
        
        snapVolume.element( by.css( '.icon-snapshot' ) ).click();
        
        element( by.css( '.fds-modal-ok' ) ).click();
        
        snapVolume.click();
        
        var window = element.all( by.css( '.slide-window-stack-slide' ) ).get( 2 );
        expect( window.getAttribute( 'style' ) ).toContain( 'left: 0%' );
        
        browser.sleep( 210 );
        
        var snapshotRows = element.all( by.css( '.snapshot-row' ) );
        snapshotRows.count().then( function( num ){
            expect( num ).toBe( 1 );
        });
    });
    
});