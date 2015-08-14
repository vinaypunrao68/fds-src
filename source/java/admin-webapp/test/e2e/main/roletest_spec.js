require( '../utils' );

describe( 'Test role effects on visual elements', function(){

    browser.addMockModule( 'user-management', mockAuth );
    browser.get( '#/' );

    var uEl = element( by.model( 'username' ) );
    uEl = uEl.element( by.tagName( 'input' ) );
    var pEl = element( by.model( 'password' ) );
    pEl = pEl.element( by.tagName( 'input' ) );
    var button = element( by.id( 'login.submit' ) );

    it ( 'should show the system management page for an admin', function(){

        login();

        browser.sleep( 200 );

        var sysNav = element( by.id( 'main.system') );

        expect( sysNav.getAttribute( 'class' ) ).not.toContain( 'ng-hide' );
        
        logout();
    });

// page removed for the time being
//    it ( 'should show the upgrade and rollback buttons for an admin', function(){
//
//        var adminlink = element( by.id( 'main.admin' ) );
//        adminlink.click();
//
//        var upgradeDiv = element( by.css( '.upgrade-panel' ) );
//        expect( upgradeDiv.getAttribute( 'class' ) ).not.toContain( 'ng-hide' );
//
//    });

// page removed for the time being
//    it ( 'should show the log and alert selection for an admin', function(){
//
//        var logPanel = element( by.css( '.log-and-alert-panel' ) );
//
//        expect( logPanel.getAttribute( 'class' ) ).not.toContain( 'ng-hide' );
//
//        logout();
//    });

    it ( 'should not show the system management page for an tenant admin', function(){

        uEl.sendKeys( 'goldman' );
        pEl.sendKeys( 'goldman' );
        button.click();

        browser.sleep( 200 );

        var sysNav = element( by.id( 'main.system') );

        expect( sysNav.getAttribute( 'class' ) ).toContain( 'ng-hide' );
    });

// removed from the UI currently
//    it ( 'should not show the upgrade and rollback buttons for a tenant admin', function(){
//
//        var adminlink = element( by.id( 'main.admin' ) );
//        adminlink.click();
//
//        var upgradeDiv = element( by.css( '.upgrade-panel' ) );
//        expect( upgradeDiv.getAttribute( 'class' ) ).toContain( 'ng-hide' );
//
//    });
//
//    it ( 'should not show the log and alert selection for a tenant admin', function(){
//
//        var logPanel = element( by.css( '.log-and-alert-panel' ) );
//
//        expect( logPanel.getAttribute( 'class' ) ).toContain( 'ng-hide' );
//
//        logout();
//    });


});
