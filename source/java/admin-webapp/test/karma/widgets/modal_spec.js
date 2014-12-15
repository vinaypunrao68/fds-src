describe( 'Modal dialog', function(){
    
    var $scope, $modal;
    
    beforeEach( function(){
        
        var fakeFilter;
        
        module( function( $provide ){
            
            $provide.value( 'translateFilter', fakeFilter );
        });
        
        fakeFilter = function( value ){
           var txts = {
               'common.l_warning': 'Warning',
               'common.l_error': 'Error',
               'common.l_confirm': 'Confirm',
               'common.l_info': 'Information',
               'common.l_ok': 'OK',
               'common.l_cancel': 'Cancel',
               'common.l_yes': 'Yes',
               'common.l_no': 'No'
           };
            
            return txts[value];
        };
        
        module( 'display-widgets' );
        module( 'templates-main' );
        
        inject( function( $compile, $rootScope ){
            
            html = '<div style="width: 500px;height: 500px;"><fds-modal show-event="show-dialog" confirm-event="confirm-dialog"></fds-modal></div>';
            $scope = $rootScope.$new();
            $modal = angular.element( html );
            $compile( $modal )( $scope );
            
            $('body').append( $modal );
            $scope.$digest();
            
            $modal = $modal.find( '.fds-modal' );
        });
    });
    
    it( 'should not be visible by default', function(){
        
        expect( $modal.css( 'display' ) ).toBe( 'none' );
        expect( $modal.hasClass( 'ng-hide' ) ).toBe( true );
    });
    
    it( 'should show an info message and be cleared', function(){
        
        var text = 'This is an awesome alert message that everyone should read!';
        var event = {type: 'INFO', text: text };
        $scope.$emit( 'show-dialog', event );
        $scope.$apply();
        expect( $modal.css( 'display' ) ).toBe( 'block' );
        expect( $modal.find( '.text' ).text().trim() ).toBe( text );
        expect( $modal.find( '.header' ).text().trim() ).toBe( 'Information' );
        expect( $modal.find( '.content-pane' ).css( 'width' ) ).toBe( '350px' );
        
        $modal.find( '.fds-modal-ok' ).click();
        
        expect( $modal.css( 'display' ) ).toBe( 'none' );
    });
    
    it( 'should show a warning message', function(){
        
        var text = 'Something is going to explode!';
        var event = { type: 'WARN', text: text };
        $scope.$emit( 'show-dialog', event );
        $scope.$apply();
        expect( $modal.css( 'display' ) ).toBe( 'block' );
        expect( $modal.find( '.text' ).text().trim() ).toBe( text );
        expect( $modal.find( '.header' ).text().trim() ).toBe( 'Warning' );
        
        $modal.find( '.fds-modal-ok' ).click();
        
        expect( $modal.css( 'display' ) ).toBe( 'none' );        
    });
    
    it( 'should show an error message', function(){
        
        var text = 'Something is exploding!';
        var event = { type: 'ERROR', text: text };
        $scope.$emit( 'show-dialog', event );
        $scope.$apply();
        expect( $modal.css( 'display' ) ).toBe( 'block' );
        expect( $modal.find( '.text' ).text().trim() ).toBe( text );
        expect( $modal.find( '.header' ).text().trim() ).toBe( 'Error' );
        
        $modal.find( '.fds-modal-ok' ).click();
        
        expect( $modal.css( 'display' ) ).toBe( 'none' );        
    });    
    
    it( 'should become larger when the message is really long', function(){
        
        var text = 'This is really long message that should cause the modal to expand to 500px instead of the default 350px size.  Therefore this message needs to be ridiculously long to ensure the spillover';
        var event = { type: 'someothername', text: text };
        $scope.$emit( 'show-dialog', event );
        $scope.$apply();
        
        expect( $modal.css( 'display' ) ).toBe( 'block' );
        expect( $modal.find( '.text' ).text().trim() ).toBe( text );
        expect( $modal.find( '.header' ).text().trim() ).toBe( 'Information' );   
        expect( $modal.find( '.content-pane' ).css( 'width' ) ).toBe( '500px' );
        
        $modal.find( '.fds-modal-ok' ).click();
        
        expect( $modal.css( 'display' ) ).toBe( 'none' );        
        
    });
    
    it( 'should handle a confirm dialog and return true when the user picks yes', function(){
        
        var confirmHandler = function( result ){
            expect( result ).toBe( true );
        };
        
        var text = 'Are you sure you want to use this UI?';
        var event = { type: 'CONFIRM', text: text };
        $scope.$emit( 'confirm-dialog', event, confirmHandler );
        $scope.$apply();
        
        expect( $modal.css( 'display' ) ).toBe( 'block' );
        expect( $modal.find( '.text' ).text().trim() ).toBe( text );
        expect( $modal.find( '.header' ).text().trim() ).toBe( 'Confirm' );
        
        $modal.find( '.fds-modal-ok' ).click();
        
        expect( $modal.css( 'display' ) ).toBe( 'none' );          
    });
    
    it( 'should handle a confirm dialog and return false when the user picks no', function(){
        
        var confirmHandler = function( result ){
            expect( result ).toBe( false );
        };
        
        var text = 'Are you sure you want to use this UI?';
        var event = { type: 'CONFIRM', text: text };
        $scope.$emit( 'confirm-dialog', event, confirmHandler );
        $scope.$apply();
        
        expect( $modal.css( 'display' ) ).toBe( 'block' );
        expect( $modal.find( '.text' ).text().trim() ).toBe( text );
        expect( $modal.find( '.header' ).text().trim() ).toBe( 'Confirm' );
        
        $modal.find( '.fds-modal-cancel' ).click();
        
        expect( $modal.css( 'display' ) ).toBe( 'none' );          
    });    
});