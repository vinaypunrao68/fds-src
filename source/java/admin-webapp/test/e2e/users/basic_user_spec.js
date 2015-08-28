require( '../utils' );

describe( 'Basic user management', function(){

    browser.get( '#/' );

    var username = {};
    var password = {};
    var confirm = {};

    var createScreen = {};
    var createLink = {};
    var saveButton = {};

    it ( 'should start with one user... admin', function(){
        
        // initializing stuff
        login();
        goto( 'users' );
        browser.sleep( 200 );
        
        username = element( by.css( '.create-user-name' ) ).element( by.tagName( 'input' ) );
        password = element( by.css( '.create-user-password' ) ).element( by.tagName( 'input' ) );
        confirm = element( by.css( '.create-user-confirm' ) ).element( by.tagName( 'input' ) );

        createScreen = element( by.css( '.create-user' ) );
        createLink = element( by.css( '.add-user-link' ) );
        saveButton = element( by.css( '.create-user-button' ));
        
        // actual test
        var users = element.all( by.css( '.user-row' ) ).then( function( elems ){
            
            expect( elems.length ).toBe( 1 );

            elems[0].element( by.css( '.user-identifier' ) ).getText().then( function( txt ){
                expect( txt ).toBe( 'admin' );
            });
        });
    });

    it ( 'should be able to create a user', function(){

        createLink.click();

        browser.sleep( 200 );

        expect( createScreen ).not.toBe( undefined );

        username.sendKeys( 'nate' );
        password.sendKeys( 'nate' );
        confirm.sendKeys( 'nate' );

        saveButton.click();

        browser.sleep( 200 );

        var users = element.all( by.css( '.user-row' ) ).then( function( elems ){

            expect( elems.length ).toBe( 2 );

            elems[1].element( by.css( '.user-identifier' ) ).getText().then( function( txt ){
                expect( txt ).toBe( 'nate' );
            });
        });

    });

    it ( 'display required by fields only when text is removed', function(){

        createLink.click();

        browser.sleep( 200 );

        var errorSpans = createScreen.all( by.css( 'span.required.ng-hide' )).then( function( elems ){
            expect( elems.length ).toBe( 3 );
        });

        username.sendKeys( 'aname' );
        username.clear();

        createScreen.all( by.css( 'span.required.ng-hide' )).then( function( elems ){
            expect( elems.length ).toBe( 2 );
        });

        username.sendKeys( 'aname' );
        username.sendKeys( protractor.Key.TAB );
        password.sendKeys( protractor.Key.TAB );

        createScreen.all( by.css( 'span.required.ng-hide' )).then( function( elems ){
            expect( elems.length ).toBe( 3 );
        });

        password.sendKeys( 'pass' );
        password.clear();

        createScreen.all( by.css( 'span.required.ng-hide' )).then( function( elems ){
            expect( elems.length ).toBe( 2 );
        });

        confirm.sendKeys( 'pass' );
        confirm.clear();

        createScreen.all( by.css( 'span.required.ng-hide' )).then( function( elems ){
            expect( elems.length ).toBe( 1 );
        });

        username.clear();
        password.clear();
        confirm.clear();
    });

    it ( 'should not allow the user to be created when passwords don\'t match', function(){

        username.sendKeys( 'aname' );
        password.sendKeys( 'pass1' );
        confirm.sendKeys( 'pass2' );

        saveButton.click();

        var errorBox = createScreen.element( by.css( '.error' ));

        expect( errorBox.getAttribute( 'class' )).not.toContain( 'ng-hide' );

        confirm.sendKeys( 'pass1' );

        expect( errorBox.getAttribute( 'class' )).toContain( 'ng-hide' );
    });

    it( 'should clear the fields out when you cancel', function(){

        var cancelButton = element( by.css( '.create-user-cancel-button' ));
        cancelButton.click();

        // make sure no new user was created
        var users = element.all( by.css( '.user-row' ) ).then( function( elems ){

            expect( elems.length ).toBe( 2 );

        });

        browser.sleep( 200 );
        
        createLink.click();

        var emptyTextCheck = function( txt ){
            expect( txt ).toBe( '' );
        };

        username.getText().then( emptyTextCheck );
        password.getText().then( emptyTextCheck );
        confirm.getText().then( emptyTextCheck );

        logout();
    });
});
