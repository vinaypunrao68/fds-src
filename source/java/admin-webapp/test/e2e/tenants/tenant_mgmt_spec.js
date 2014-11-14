require( '../utils' );

describe( 'Tenant management', function(){

    browser.get( '#/' );

    login();
    goto( 'tenants' );

    var createPage = {};
    var createButton = {};

    var nameEl = {};

    it ( 'should start with no tenants', function(){

        // initializing stuff
        login();
        goto( 'tenants' );

        createPage = element( by.css( '.create-tenant' ) );
        createButton = createPage.element( by.css( '.create-tenant-button' ) );

        nameEl = createPage.element( by.css( '.fui-input' ) ).element( by.tagName( 'input' ) );
        
        // test
        var tenants = element.all( by.css( '.tenant-row' ) ).then( function( elems ){
            expect( elems.length ).toBe( 0 );
        });
    });

    it ( 'should be able to create a tenant', function(){

        var createLink = element( by.css( '.create-tenant-link' ) );
        createLink.click();


        expect( createPage.getAttribute( 'class' ) ).not.toContain( 'ng-hide' );
        nameEl.sendKeys( 'CoolTenant' );

        createButton.click();
        browser.sleep( 200 );

        element.all( by.css( '.tenant-row' ) ).then( function( elems ){

            expect( elems.length ).toBe( 1 );

            var name = elems[0].element( by.css( '.tenant-name' ) ).getText().then( function( txt ){
                expect( txt ).toBe( 'CoolTenant' );
            });
        });
    });

});
