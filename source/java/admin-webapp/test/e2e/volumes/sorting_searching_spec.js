require( '../utils' );

describe( 'Testing the timeline settings', function(){
    
    var volPage = element( by.css( '.volume-page' ) );
    
    clean();
    
    it( 'should be able to sort the volumes by name', function(){
        login();
        
        goto( 'volumes' );
        
        createVolume( 'abcd', 'object', {priority: 8, capacity: 90, limit: 300} );
        createVolume( 'zyxwv', 'object', {priority: 2, capacity: 90, limit: 300} );
        
        volPage.all( by.css( '.volume-row' ) ).count().then( function( total ){
            expect( total ).toBe( 2 );
        });
        
        var first = volPage.all( by.css( '.volume-row' )).get(0);
        var last = volPage.all( by.css( '.volume-row' )).last();
        
        first.element( by.css( '.name' ) ).getText().then( function( text ){
            expect( text ).toBe( 'abcd' );
        });
        
        last.element( by.css( '.name' ) ).getText().then( function( text ){
            expect( text ).toBe( 'zyxwv' );
        });
        
        var nameHeader = volPage.all( by.css( '.sort-helper' ) ).first();
        nameHeader.click();
        nameHeader.click();
        
        first = volPage.all( by.css( '.volume-row' )).get(0);
        last = volPage.all( by.css( '.volume-row' )).last();
        
        first.element( by.css( '.name' ) ).getText().then( function( text ){
            expect( text ).toBe( 'zyxwv' );
        });
        
        last.element( by.css( '.name' ) ).getText().then( function( text ){
            expect( text ).toBe( 'abcd' );
        });
    });
    
    it( 'should be able to sort by priority', function(){
        
        var pHeader = volPage.all( by.css( '.sort-helper' )).get( 5 );
        pHeader.click();
        
        var first = volPage.all( by.css( '.volume-row' )).get(0);
        var last = volPage.all( by.css( '.volume-row' )).last();
        
        first.element( by.css( '.name' ) ).getText().then( function( text ){
            expect( text ).toBe( 'zyxwv' );
        });
        
        last.element( by.css( '.name' ) ).getText().then( function( text ){
            expect( text ).toBe( 'abcd' );
        });
        
        pHeader.click();
        
        var first = volPage.all( by.css( '.volume-row' )).get(0);
        var last = volPage.all( by.css( '.volume-row' )).last();
        
        first.element( by.css( '.name' ) ).getText().then( function( text ){
            expect( text ).toBe( 'abcd' );
        });
        
        last.element( by.css( '.name' ) ).getText().then( function( text ){
            expect( text ).toBe( 'zyxwv' );
        });        
    });
    
    it( 'should filter the table by typing in the search box', function(){
        
        var searchBox = volPage.element( by.model( 'searchText' ) );
        searchBox = searchBox.element( by.tagName( 'input' ) );
        
        searchBox.clear();
        searchBox.sendKeys( 'abc' );
        
        var rows = volPage.all( by.css( '.volume-row' ) );
        
        rows.count().then( function( rows ){
            expect( rows ).toBe( 1 );
        });
        
        rows.first().element( by.css( '.name' ) ).getText().then( function( text ){
            expect( text ).toBe( 'abcd' );
        });
        
        searchBox.clear();
        
        rows = volPage.all( by.css( '.volume-row' ) );
        
        rows.count().then( function( rows ){
            expect( rows ).toBe( 2 );
        });
        
        searchBox.sendKeys( 'yxw' );
        
        rows = volPage.all( by.css( '.volume-row' ) );
        
        rows.count().then( function( rows ){
            expect( rows ).toBe( 1 );
        });
        
        rows.first().element( by.css( '.name' ) ).getText().then( function( text ){
            expect( text ).toBe( 'zyxwv' );
        });
        
        searchBox.clear();
        searchBox.sendKeys( '999999' );
        
        rows = volPage.all( by.css( '.volume-row' ) );
        
        rows.count().then( function( rows ){
            expect( rows ).toBe( 0 );
        });
        
        searchBox.clear();
        
        deleteVolume( 0 );
        deleteVolume( 0 );
    });
});