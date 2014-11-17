require( './mockAuth' );
require( './mockSnapshot' );
require( './mockVolume' );
require( './mockTenant' );

addModule = function( moduleName, module ){

    if ( browser.params.mockServer === true ){
        browser.addMockModule( moduleName, module );
    }
};

addModule( 'user-management', mockAuth );
addModule( 'volume-management', mockVolume );
addModule( 'qos', mockSnapshot );
addModule( 'tenant-management', mockTenant );

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

createTenant = function( name ){
    
    goto( 'tenants' );
    var createLink = element( by.css( '.create-tenant-link' ) );
    createLink.click();

    var nameEl = element( by.css( '.tenant-name-input' )).element( by.tagName( 'input' ));
    nameEl.sendKeys( name );

    element( by.css( '.create-tenant-button' ) ).click();
    browser.sleep( 200 );
};
