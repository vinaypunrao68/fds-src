require( '../utils' );

describe( 'Testing volume creation permutations', function(){

//    browser.get( '#/' );
    var snapshotPolicyEdit;
    var saveButton;
    var snapVolume;
    var spinners;
    var checks;
    var combos;
    
    it ( 'should be able to create a volume with a snapshot policy', function(){
        
        // create a volume with a daily and monthly snapshot policy
        login();
        goto( 'volumes' );
        browser.sleep( 220 );
        element( by.css( '.new_volume' ) ).click();
        browser.sleep( 200 );
        
        snapshotPolicyEdit = element( by.css( '.edit-snapshot-policies' ) );
        snapshotPolicyEdit.click();
        
        // make sure the displays have been toggled
        element.all( by.css( '.policy-value-display' ) ).each( function( el ){
            expect( el.getAttribute( 'class' ) ).toContain( 'ng-hide' );
        });
        
        // use daily for 2 days and monthly for 3 weeks
        checks = element.all( by.css( '.tristatecheck' ));
        checks.get( 0 ).element( by.css( '.unchecked' )).click();
        checks.get( 2 ).element( by.css( '.unchecked' )).click();
        
        spinners = element.all( by.css( '.retention-spinner' ) );
        combos = element.all( by.css( '.retention-dropdown' ) );
        spinners.get( 0 ).element( by.tagName( 'input' )).clear().sendKeys( '2' );
        combos.get( 0 ).click();
        combos.get( 0 ).all( by.tagName( 'li' )).get( 1 ).click();
        
        spinners.get( 2 ).element( by.tagName( 'input' )).clear().sendKeys( '3' );
        combos.get( 2 ).click();
        combos.get( 2 ).all( by.tagName( 'li' )).get( 2 ).click();
        
        saveButton = element( by.css( '.save-snapshot-policies' ));
        saveButton.click();
        
        // give it a name
        element( by.css( '.volume-name' )).clear().sendKeys( 'SnapshotVolume' );
        element.all( by.buttonText( 'Create Volume' )).get( 0 ).click();
        
    });
    
    it ( 'should be able to be edited', function(){
        
        // find our volume
        snapVolume = element( by.cssContainingText( '.volume-row', 'SnapshotVolume' ));
        snapVolume.element( by.css( '.fui-new' ) ).click();
        
        // verify the snapshot policy is there
        expect( checks.get( 0 ).element( by.css( '.checked' )).getAttribute( 'class' ) ).not.toContain( 'ng-hide' );
        expect( checks.get( 2 ).element( by.css( '.checked' )).getAttribute( 'class' ) ).not.toContain( 'ng-hide' );
        
        var texts = element.all( by.css( '.policy-value-display' ) );
        texts.get( 0 ).getText().then( function( txt ){
            expect( txt ).toContain( '2 days' );
        });
        
        texts.get( 2 ).getText().then( function( txt ){
            expect( txt ).toContain( '3 weeks' );
        });
        
        // edit it
        snapshotPolicyEdit.click();
        checks.get( 0 ).element( by.css( '.checked' ) ).click();
        checks.get( 2 ).element( by.css( '.checked' ) ).click();
        checks.get( 1 ).element( by.css( '.unchecked' ) ).click();
        
        combos.get( 1 ).click();
        combos.get( 1 ).all( by.tagName( 'li' ) ).get( 0 ).click();
        
        saveButton.click();
        
        element.all( by.buttonText( 'Edit Volume' )).get( 0 ).click();
        
        // click "new" and make sure the policy isn't there
        element( by.css( '.new_volume' ) ).click();
        
        expect( checks.get( 0 ).element( by.css( '.checked' ) ).getAttribute( 'class' ) ).toContain( 'ng-hide' );
        expect( checks.get( 1 ).element( by.css( '.checked' ) ).getAttribute( 'class' ) ).toContain( 'ng-hide' );
        expect( checks.get( 2 ).element( by.css( '.checked' ) ).getAttribute( 'class' ) ).toContain( 'ng-hide' );
        expect( checks.get( 3 ).element( by.css( '.checked' ) ).getAttribute( 'class' ) ).toContain( 'ng-hide' );
        
        element.all( by.buttonText( 'Cancel' ) ).get( 0 ).click();
        
        snapVolume.element( by.css( '.fui-new' ) ).click();
        
        browser.sleep( 200 );

// mock service isn't accepting edits yet        
//        expect( checks.get( 1 ).element( by.css( '.checked' )).getAttribute( 'class' ) ).not.toContain( 'ng-hide' );
//        
//        var texts = element.all( by.css( '.policy-value-display' ) );
//        texts.get( 1 ).getText().then( function( txt ){
//            expect( txt ).toContain( '1 days' );
//        });
        
    });
    
    it ( 'should be able to take a snapshot and see it in the list', function(){
    });
    
});