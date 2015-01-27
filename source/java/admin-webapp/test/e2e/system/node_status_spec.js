require( '../utils' );

describe( 'Testing the system screen', function(){

    browser.get( '#/' );
    
    it( 'should have one node and states should match necessary icons', function(){

        login();
        goto( 'system' );

        browser.sleep( 1200 );
        
        var rows = element.all( by.css( '.node-item' ) );
        
        rows.count().then( function( rowCount ){
            expect( rowCount ).toBe( 1 );
        });
        
        // check the name
        var node1 = rows.get( 0 );
        
        node1.element( by.css( '.node-name' ) ).getText().then( function( txt ){
            
            expect( txt ).toBe( 'awesome-node' );
        });
        
        expect( node1.element( by.css( '.ams span' ) ).getAttribute( 'class' ) ).toContain( 'icon-excellent' );
        expect( node1.element( by.css( '.pms span' ) ).getAttribute( 'class' ) ).toContain( 'icon-issues' );
        expect( node1.element( by.css( '.dms span' ) ).getAttribute( 'class' ) ).toContain( 'icon-nothing' );
        expect( node1.element( by.css( '.sms span' ) ).getAttribute( 'class' ) ).toContain( 'icon-warning' );
        
    });
});