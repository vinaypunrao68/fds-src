describe( 'Switch button functionality', function(){
    
    var $scope, $switch;
    
    beforeEach( function(){
        
        module( 'form-directives' );
        module( 'templates-main' );
        
        inject( function( $compile, $rootScope ){
            
            var html = '<switch ng-model="switchState" disabled="disabledState" on-text="ON" off-text="OFF"></switch>';
            $scope = $rootScope.$new();
            $switch = angular.element( html );
            $compile( $switch )( $scope );
            
            $scope.switchState = true;
            $scope.disabledState = false;
            
            $('body').append( $switch );
            $scope.$digest();
            
        });
    });
   
    it( 'should default to on and be blue', function(){
        
        var switchButton = $switch.find( '.switch-button' );
        expect( $switch.hasClass( 'on' ) ).toBe( true );
        expect( $(switchButton).css( 'right' )).toBe( '2px' );
        
        var veil = $switch.find( '.veil' );
        
        expect( $(veil).hasClass( 'ng-hide' ) ).toBe( true );
        
    });
    
    it( 'should switch to off when clicked', function(){
        
        var switchButton = $switch.find( '.switch-button' );
        switchButton.click();
        
        expect( $scope.switchState ).toBe( false );
        expect( $switch.hasClass( 'on' ) ).toBe( false );
        expect( $(switchButton).css( 'left' )).toBe( '2px' );
        
    });
    
    it ( 'should switch back to on when the model is changed', function(){
        
        $scope.switchState = true;
        $scope.$apply();
        
        var switchButton = $switch.find( '.switch-button' );
        expect( $switch.hasClass( 'on' ) ).toBe( true );
        expect( $(switchButton).css( 'right' )).toBe( '2px' );
        expect( $scope.switchState ).toBe( true );
    });
    
    it( 'should show the veil when disabled', function(){
        
        $scope.disabledState = true;
        $scope.$apply();
        
        var veil = $switch.find( '.veil' );
        expect( $(veil).hasClass( 'ng-hide' ) ).toBe( false );
    });
});