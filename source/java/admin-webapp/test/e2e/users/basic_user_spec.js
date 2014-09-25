require( '../utils' );

describe( 'Basic user management', function(){

    browser.get( '#/' );

    login();
    goto( 'users' );

    it ( 'should start with one user... admin', function(){

        var users = element.all( by.css( '.user-row' ) ).then( function( elems ){

            expect( elems.length ).toBe( 1 );

            elems[0].element( by.css( '.user-identifier' ) ).getText().then( function( txt ){
                expect( txt ).toBe( 'admin' );
            });
        });
    });

    it ( 'should be able to create a user', function(){

        var createLink = element( by.css( '.add-user-link' ) );
        createLink.click();

        browser.sleep( 200 );

        var createScreen = element( by.css( '.create-user' ) );
        expect( createScreen ).not.toBe( undefined );

        var username = element( by.css( '.create-user-name' ) ).element( by.tagName( 'input' ) );
        var password = element( by.css( '.create-user-password' ) ).element( by.tagName( 'input' ) );
        var confirm = element( by.css( '.create-user-confirm' ) ).element( by.tagName( 'input' ) );

        username.sendKeys( 'nate' );
        password.sendKeys( 'nate' );
        confirm.sendKeys( 'nate' );

        var saveButton = element( by.css( '.create-user-button' ));
        saveButton.click();

        browser.sleep( 200 );

        var users = element.all( by.css( '.user-row' ) ).then( function( elems ){

            expect( elems.length ).toBe( 2 );

            elems[1].element( by.css( '.user-identifier' ) ).getText().then( function( txt ){
                expect( txt ).toBe( 'nate' );
            });
        });
    });

});
