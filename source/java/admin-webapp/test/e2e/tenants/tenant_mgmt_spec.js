require( '../utils' );

describe( 'Tenant management', function(){

    var mockTenant = function(){
        angular.module( 'tenant-management' ).factory( '$tenant_service', function(){

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

            service.attachUser = function( tenant, userId, callback, failure ){

                return;
            };

            return service;

        });
    };

    browser.addMockModule( 'tenant-management', mockTenant );
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

        expect( createPage.getAttribute( 'class' ) ).toContain( 'ng-hide' );

        element.all( by.css( '.tenant-row' ) ).then( function( elems ){

            expect( elems.length ).toBe( 1 );

            var name = elems[0].element( by.css( '.tenant-name' ) ).getText().then( function( txt ){
                expect( txt ).toBe( 'CoolTenant' );
            });
        });
    });

});
