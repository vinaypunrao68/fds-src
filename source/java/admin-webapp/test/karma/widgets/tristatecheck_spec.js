describe( 'Tri-State checkbox', function(){
    
    var $scope, $checkbox;
    
    beforeEach( function(){
        
        module( 'form-directives' );
        module( 'templates-main' );
        
        inject( function( $compile, $rootScope ){
            
            var html = '<tri-state-check check-state="state" enabled="enabled"></tri-state-check>';
            $scope = $rootScope.$new();
            $checkbox = angular.element( html );
            $compile( $checkbox )( $scope );
            
            $scope.state = false;
            $scope.enabled = true;
            
            $('body').append( $checkbox );
            $scope.$digest();
        });
    });
    
    it( 'should be checkable', function(){
        
        expect( $($checkbox.find( '.unchecked' )).css( 'display' ) ).toBe( 'block' );
        expect( $($checkbox.find( '.checked' )).css( 'display' ) ).toBe( 'none' );
        expect( $($checkbox.find( '.partial' )).css( 'display' ) ).toBe( 'none' );
        
        $checkbox.find( '.unchecked' ).click();
        
        expect( $($checkbox.find( '.unchecked' )).css( 'display' ) ).toBe( 'none' );
        expect( $($checkbox.find( '.checked' )).css( 'display' ) ).toBe( 'block' );
        expect( $($checkbox.find( '.partial' )).css( 'display' ) ).toBe( 'none' );
        expect( $scope.state ).toBe( true );
    });
    
    it( 'should be uncheckable', function(){
        $checkbox.find( '.unchecked' ).click();
        $checkbox.find( '.checked' ).click();
        
        expect( $scope.state ).toBe( false );
        expect( $($checkbox.find( '.unchecked' )).css( 'display' ) ).toBe( 'block' );
        expect( $($checkbox.find( '.checked' )).css( 'display' ) ).toBe( 'none' );
        expect( $($checkbox.find( '.partial' )).css( 'display' ) ).toBe( 'none' );
    });
    
    it( 'should be settable by changing the scope variable', function(){
        
        $scope.state = true;
        $scope.$apply();
        
        expect( $($checkbox.find( '.unchecked' )).css( 'display' ) ).toBe( 'none' );
        expect( $($checkbox.find( '.checked' )).css( 'display' ) ).toBe( 'block' );
        expect( $($checkbox.find( '.partial' )).css( 'display' ) ).toBe( 'none' );
        
        $scope.state = false;
        $scope.$apply();
        
        expect( $($checkbox.find( '.unchecked' )).css( 'display' ) ).toBe( 'block' );
        expect( $($checkbox.find( '.checked' )).css( 'display' ) ).toBe( 'none' );
        expect( $($checkbox.find( '.partial' )).css( 'display' ) ).toBe( 'none' );        
    });
    
    it( 'should be settable to partial, and switch to unchecked after clicked', function(){
        
        $scope.state = 'partial';
        $scope.$apply();
        
        expect( $($checkbox.find( '.unchecked' )).css( 'display' ) ).toBe( 'none' );
        expect( $($checkbox.find( '.checked' )).css( 'display' ) ).toBe( 'none' );
        expect( $($checkbox.find( '.partial' )).css( 'display' ) ).toBe( 'block' );     
        
        $checkbox.find( '.partial' ).click();
        
        expect( $($checkbox.find( '.unchecked' )).css( 'display' ) ).toBe( 'block' );
        expect( $($checkbox.find( '.checked' )).css( 'display' ) ).toBe( 'none' );
        expect( $($checkbox.find( '.partial' )).css( 'display' ) ).toBe( 'none' ); 
    });
});