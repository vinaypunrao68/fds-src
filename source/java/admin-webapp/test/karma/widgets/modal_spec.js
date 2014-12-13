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
               'common.l_info': 'Info',
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
        
        $modal.find( '.fds-modal-ok' ).click();
        
        expect( $modal.css( 'display' ) ).toBe( 'none' );
    });
});