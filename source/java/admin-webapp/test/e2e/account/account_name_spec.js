require( '../utils' );

describe( 'Account username page', function(){

    browser.get( '#/' );

    it( 'Should show the correct user name in the account page', function(){

        login();

        var logoutMenu = element( by.id( 'main.usermenu' ) );
        var accountEl = element( by.id( 'main.usermenu' ) ).all( by.tagName( 'li' )).get( 0 );

        logoutMenu.click();
        accountEl.click();
        browser.sleep( 200 );


        var usernameEl = element( by.css( '.username') );
        usernameEl.getText().then( function( txt ){
            expect( txt ).toBe( 'admin' );
        });
    });

});
