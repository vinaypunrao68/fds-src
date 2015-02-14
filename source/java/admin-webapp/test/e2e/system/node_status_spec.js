require( '../utils' );

describe( 'Testing the system screen', function(){

    browser.get( '#/' );
    
    var nodePage;
    var addPage;
    
    var checkNodeCount = function( detached, num ){
        
        var rows;
        
        if ( detached === false ){
            rows = nodePage.all( by.css( '.node-item' ) );
        }
        else {
            rows = addPage.all( by.css( '.detached-nodes' ) );
        }
        
        rows.count().then( function( rowCount ){
            expect( rowCount ).toBe( num );
        });
        
        return rows;
    };
    
    var gotoAddPage = function(){
        
        var addNode = nodePage.element( by.css( '.add_node' ) );
        addNode.click();
        
        browser.sleep( 220 );
    };
    
    var clickAddNode = function(){
        var addButton = addPage.element( by.css( '.add-node-button' ));
        addButton.click();
    };
    
    var clickRemove = function( index ){
        var newRow = nodePage.all( by.css( '.node-item' )).get( index );
        var removeButton = newRow.element( by.css( '.remove' ));
        removeButton.click();
    };
    
    it( 'should have one node and states should match necessary icons', function(){

        login();
        goto( 'system' );

        browser.sleep( 200 );
        
        nodePage = element( by.css( '.nodecontainer' ) );
        addPage = element( by.css( '.add-node-dialog-page' ) );
        
        var rows = checkNodeCount( false, 1 );
        
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
        
        gotoAddPage();
        
        var viewPage = element.all( by.css( '.slide-window-stack-slide' ) ).get( 1 );
        expect( viewPage.getAttribute( 'style' ) ).toContain( '0%' );
        
    });
    
    it ( 'should have one detached node in the list', function(){
        
        checkNodeCount( true, 1 );
        
        addPage.element( by.css( '.node-name' ) ).getText().then( function( txt ){
            expect( txt ).toBe( 'awesome-new-node' );
        });
    });
    
    it( 'should add the node and switch back to the node list, now listing both nodes', function(){
        
        clickAddNode();
        
        browser.sleep( 220 );
        
        var listPage = element.all( by.css( '.slide-window-stack-slide' ) ).get( 0 );
        expect( listPage.getAttribute( 'style' ) ).toContain( '0%' );
        
        checkNodeCount( false, 2 );
    });
    
    it( 'should remove a node from the list when the remove button is clicked', function(){
        
        clickRemove( 1 );
        
        element( by.css( '.fds-modal-ok' ) ).click();
        
        checkNodeCount( false, 1 );
    });
    
    it ( 'should give you a warning if you try to add only a portion of available nodes', function(){
        
        clickRemove( 0 );
        
        clickOk();
        
        gotoAddPage();
        
        checkNodeCount( true, 2 );
        
        clickAddNode();
//        browser.sleep( 30000 );
        expect( element( by.css( '.fds-modal' )).getAttribute( 'class' ) ).not.toContain( 'ng-hide' );
        
        clickCancel();
        
        checkNodeCount( true, 2 );
        
        clickAddNode();
        
        clickOk();
        
        browser.sleep( 220 );
        
        var listPage = element.all( by.css( '.slide-window-stack-slide' ) ).get( 0 );
        expect( listPage.getAttribute( 'style' ) ).toContain( '0%' );
        
        checkNodeCount( false, 1 );
    });
});