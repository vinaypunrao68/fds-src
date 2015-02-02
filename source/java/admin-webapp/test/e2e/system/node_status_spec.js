require( '../utils' );

describe( 'Testing the system screen', function(){

    browser.get( '#/' );
    
    var nodePage;
    var addPage;
    
    it( 'should have one node and states should match necessary icons', function(){

        login();
        goto( 'system' );

        browser.sleep( 200 );
        
        nodePage = element( by.css( '.nodecontainer' ) );
        
        var rows = nodePage.all( by.css( '.node-item' ) );
        
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
    
    it ( 'should go to the add node screen when the button is pressed', function(){
        
        var addNode = nodePage.element( by.css( '.add_node' ) );
        addNode.click();
        
        browser.sleep( 220 );
        
        
        var viewPage = element.all( by.css( '.slide-window-stack-slide' ) ).get( 1 );
        expect( viewPage.getAttribute( 'style' ) ).toContain( '0%' );
        
    });
    
    it ( 'should have one detached node in the list', function(){
        
        addPage = element( by.css( '.add-node-dialog-page' ) );
        
        var rows = addPage.all( by.css( '.detached-nodes' ) );
        
        rows.count().then( function( num ){
            
            expect( num ).toBe( 1 );
        });
        
        addPage.element( by.css( '.node-name' ) ).getText().then( function( txt ){
            expect( txt ).toBe( 'awesome-new-node' );
        });
    });
    
    it( 'should add the node and switch back to the node list, now listing both nodes', function(){
        
        var addButton = addPage.element( by.css( '.add-node-button' ));
        addButton.click();
        
        browser.sleep( 220 );
        
        var listPage = element.all( by.css( '.slide-window-stack-slide' ) ).get( 0 );
        expect( listPage.getAttribute( 'style' ) ).toContain( '0%' );
        
        var rows = nodePage.all( by.css( '.node-item' ) );
        
        rows.count().then( function( rowCount ){
            expect( rowCount ).toBe( 2 );
        });
    });
    
    it( 'should remove a node from the list when the remove button is clicked', function(){
        
        var newRow = nodePage.all( by.css( '.node-item' )).get( 1 );
        var removeButton = newRow.element( by.css( '.remove' ));
        removeButton.click();
        
        element( by.css( '.fds-modal-ok' ) ).click();
        
        var rows = nodePage.all( by.css( '.node-item' ) );
        
        rows.count().then( function( rowCount ){
            expect( rowCount ).toBe( 1 );
        });
    });
});