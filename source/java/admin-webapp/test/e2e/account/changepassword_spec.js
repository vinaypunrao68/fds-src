require( '../utils' );

describe( 'Test change password scenarios', function(){

    browser.get( '#/' );


    // helper to send stuff into the password fields
    var changepassword = function( newpass, confirm ){

        var logoutMenu = element( by.id( 'main.usermenu' ) );
        var changeEl = element( by.id( 'main.usermenu' ) ).all( by.tagName( 'li' ) );

        logoutMenu.click();
        changeEl.get( 1 ).click();
        browser.sleep( 200 );

        var accountPage = element( by.css( '.account-page' ) );

        expect( accountPage ).not.toBe( undefined );

        var changeLink = element( by.css( '.change-password' ) );
        changeLink.click();

        var pass = element( by.model( 'newPassword' ) );
        var confirm = element( by.model( 'confrimPassword' ) );

        pass.sendKeys( newpass );
        confirm.sendKeys( confirm );

        var saveButton = element( by.buttonText( 'Save' ) );

        saveButton.click();
    };

    it( 'Should be able to change the password', function(){

        login();

        changepassword( 'newpass', 'newpass' );

        var changeLink = element( by.css( '.change-password' ) );
        expect( changeLink.getAttribute( 'class' ) ).not.toContain( 'ng-hide' );

        logout();

        var uEl = element( by.model( 'username' ) );
        var pEl = element( by.model( 'password' ) );
        var button = element( by.id( 'login.submit' ) );

        uEl.clear();
        pEl.clear();
        uEl.sendKeys( 'admin' );
        pEl.sendKeys( 'newpass' );

        button.click();
        browser.sleep( 200 );

        expect( mainEl.getAttribute( 'class' ) ).not.toContain( 'ng-hide' );

        // change it back
        changepassword( 'admin', 'admin' );
    });

    it( 'should fail to submit if both passwords don\'t match', function(){

        changepassword( 'onepass', 'twopass' );

        var alertBox = element( by.css( 'error' ) );
        expect( alertBox.getAttribute( 'class' ) ).toContain( 'ng-show' );

    });

});
