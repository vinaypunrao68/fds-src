describe( 'Show/Hide panel functionality', function(){
    
    var scope, $panel;
    
    beforeEach( function(){
        module( 'display-widgets' );
        module( 'templates-main' );
    });
    
    beforeEach(inject( function( $compile, $rootScope ) {
            
            var html = '<show-hide-panel show-text="Show Advanced Options" hide-text="Hide Advanced Options"></show-hide-panel>';
            scope = $rootScope.$new();
            $panel = angular.element( html );
            $compile( $panel )( scope );
            
            $('body').append( $panel );
            scope.$digest();
    }));
    
    it( 'should start out showing the \'show\' text', function(){
        
        var label = $panel.find( '.show-hide-panel-label' );
        expect( $panel.text().trim() ).toBe( 'Show Advanced Options' );
        expect( $panel.children().scope().panelText ).toBe( 'Show Advanced Options' );
    });
    
    it( 'should show the contents when the label is clicked', function(){
        
        $panel.children().scope().showPanel();
        
        expect( $panel.children().scope().panelText ).toBe( 'Hide Advanced Options' );
        expect( $panel.children().scope().isOpen ).toBe( true );

    });
});