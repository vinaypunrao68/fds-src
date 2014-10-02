require( '../utils' );

describe( 'Test login permutations', function(){

    browser.addMockModule( 'user-management', mockAuth );
    browser.get( '#/' );

    var uEl = element( by.model( 'username' ) );
    uEl = uEl.element( by.tagName( 'input' ) );
    var pEl = element( by.model( 'password' ) );
    pEl = pEl.element( by.tagName( 'input' ) );
    var button = element( by.id( 'login.submit' ) );
    var errorEl = element( by.css( '.alert-info' ) );
    var mainEl = element( by.id( 'main.content' ) );
//    var logoutMenu = element( by.id( 'main.usermenu' ) );
//    var logout = element( by.id( 'main.usermenu' ) ).all( by.tagName( 'li' ) ).first();

    it( 'should say FDS', function() {
        expect( browser.getTitle() ).toEqual( 'Formation' );
    });

    it ( 'should not allow me to view the main page when logged out', function(){
        browser.get( '#/volumes' );
        expect( mainEl.getAttribute( 'class' ) ).toContain( 'ng-hide' );
        browser.get( '#/' );
    });

    it( 'should show auth failure for bad credentials', function(){

        uEl.sendKeys( 'admin' );
        pEl.sendKeys( 'badpassword' );
        expect( errorEl.getAttribute( 'class' ) ).toContain( 'ng-hide' );
        button.click();

        browser.sleep( 200 );
        expect( mainEl.getAttribute( 'class' ) ).toContain( 'ng-hide' );
        expect( errorEl.getAttribute( 'class' ) ).toBe( 'alert alert-info ng-binding' );
    });
    
    it( 'should clear the error once I start typing', function(){
        
        uEl.sendKeys( 'test' );
        expect( errorEl.getAttribute( 'class' ) ).toContain( 'ng-hide' );
    });

    it( 'should log in and get to the main screen', function(){

        login();
        expect( mainEl.getAttribute( 'class' ) ).not.toContain( 'ng-hide' );
    });

    it ( 'should log out successfully', function() {

        logout();
        browser.sleep( 200 );

        expect( mainEl.getAttribute( 'class' ) ).toContain( 'ng-hide' );
    });
});
