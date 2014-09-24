require( './mockAuth' );

browser.addMockModule( 'user-management', mockAuth );

login = function(){
    browser.get( '#/' );

    var uEl = element( by.model( 'username' ) );
    uEl = uEl.element( by.tagName( 'input' ) );
    var pEl = element( by.model( 'password' ) );
    pEl = pEl.element( by.tagName( 'input' ) );
    var button = element( by.id( 'login.submit' ) );

    uEl.clear();
    pEl.clear();
    uEl.sendKeys( 'admin' );
    pEl.sendKeys( 'admin' );

    button.click();
    browser.sleep( 200 );
};

logout = function(){

    var logoutMenu = element( by.id( 'main.usermenu' ) );
    var logoutEl = element( by.id( 'main.usermenu' ) ).all( by.tagName( 'li' ) ).last();

    logoutMenu.click();
    logoutEl.click();
    browser.sleep( 200 );
};

goto = function( tab ){

    var tab = element( by.id( 'main.' + tab ) );
    tab.click();
    browser.sleep( 200 );
};
