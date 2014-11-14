require( '../utils' );

describe( 'Edit user', function(){

    browser.get( '#/' );
    
    var username;
    var password;
    var confirm;
    var userRow;
    var editPage;
    var tenantDropdown;
    var unc;
    var duke;
    
    it ( 'should be able to click edit and see new page', function(){
        
        login();
        createTenant( 'duke' );
        createTenant( 'northcarolina' );
        
        goto( 'users' );
        browser.sleep( 200 );
        
        element( by.css( '.add-user-link' ) ).click();
        editPage = element.all( by.css( '.slide-window-stack-slide' ) ).get(1);
        
        username = editPage.element( by.css( '.create-user-name' ) ).element( by.tagName( 'input' ) );
        password = editPage.element( by.css( '.create-user-password' ) ).element( by.tagName( 'input' ) );
        confirm = editPage.element( by.css( '.create-user-confirm' ) ).element( by.tagName( 'input' ) );
        tenantDropdown = editPage.element( by.css( '.tenant-dropdown' ));
        unc = tenantDropdown.element( by.cssContainingText( 'li', 'northcarolina' ));
        duke = tenantDropdown.element( by.cssContainingText( 'li', 'duke' ));
        
        username.sendKeys( 'flipflopper' );
        password.sendKeys( 'password' );
        confirm.sendKeys( 'password' );
        
        tenantDropdown.click();
        unc.click();
        
        element( by.css( '.create-user-button' ) ).click();
        
        browser.sleep( 200 );
        
        userRow = element( by.cssContainingText( '.user-row', 'flipflopper' ));
        
        userRow.element( by.css( '.tenant-name' ) ).getText().then( function( txt ){
            expect( txt ).toBe( 'northcarolina' );
        });
        
        userRow.element( by.css( '.icon-edit' ) ).click();
        
        expect( editPage.getAttribute( 'style' ) ).toContain( '0%' );
        
    });
    
    it ( 'should not allow the user to edit name or password fields', function(){
        
        expect( username.getAttribute( 'disabled' )).toBe( 'true' );
        expect( password.getAttribute( 'disabled' )).toBe( 'true' );
        expect( element( by.css( '.password-confirmation-parent' )).getAttribute( 'class' )).toContain( 'ng-hide' );
        
    });
    
    it ( 'should allow the tenancy to be changed', function(){
        
        tenantDropdown.click();
        duke.click();
        
        element( by.css( '.create-user-button' ) ).click();
        
        browser.sleep( 200 );
        
        userRow.element( by.css( '.tenant-name' )).getText().then( function( txt ){
            expect( txt ).toBe( 'duke' );
        });
    });
});