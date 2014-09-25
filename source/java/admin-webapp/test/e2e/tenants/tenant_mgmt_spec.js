require( '../utils' );

describe( 'Tenant management', function(){

    browser.get( '#/' );

    var mockTenant = function(){
        angular.module( 'tenant-management' ).factory( '$tenant_api', function(){

            var service = {};

            service.tenants = [];

            service.getTenants = function( callback, failure ){

                if ( angular.isDefined( callback ) ){
                    callback( service.tenants );
                }
            };

            service.createTenant = function( tenant, callback, failure ){

                tenant.id = (new Date()).getTime();
                service.tenants.push( tenant );

                if ( angular.isDefined( callback ) ){
                    callback( tenant );
                }
            };

            return service;

        });
    };

    browser.addMockModule( 'tenant-management', mockTenant );

    login();
    goto( 'tenants' );

    it ( 'should start with no tenants', function(){

        var tenants = element.all( by.css( '.tenant-table tbody tr' ) ).then( function( elems ){
            expect( elems.length ).toBe( 0 );
        });
    });

    it ( 'should be able to create a tenant', function(){

        browser.sleep( 200 );
        var createLink = element( by.cssContainingText( '.pull-right', 'Create a Tenant' ) );
        createLink.click();

        var createPage = element( by.css( '.create-tenant' ) );

        expect( createPage.getAttribute( 'class' ) ).not.toContain( 'ng-hide' );

        var nameEl = createPage.element( by.css( '.fui-input' ) );
        nameEl = nameEl.element( by.tagName( 'input' ) );

        nameEl.sendKeys( 'CoolTenant' );

        var createButton = createPage.element( by.buttonText( 'Create Tenant' ) );
        createButton.click();

        expect( createPage.getAttribute( 'class' ) ).toContain( 'ng-hide' );

        var tenants = element.all( by.css( '.tenant-table tbody tr' ) ).then( function( elems ){
            expect( elems.length ).toBe( 1 );

            var name = elems[0].element( by.css( '.tenant-name' ) );
            name.getText().then( function( txt ){
                expect( txt ).toBe( 'CoolTenant' );
            });
        });
    });

});
