describe( 'Testing the spinner widget', function(){
    
    var $scope, $spinner;
    
    beforeEach( function(){

        module( function( $provide ){
            $provide.value( '$interval', function( call, time ){
                setInterval( call, time );
                return {};
            });
        });
        module( 'form-directives' );
        module( 'templates-main' );
        
        inject( function( $compile, $rootScope ){
            
            var html = '<spinner value="value" min="min" max="max" step="step"></spinner>';
            $scope = $rootScope.$new();
            $spinner = angular.element( html );
            $compile( $spinner )( $scope );
            
            $scope.min = 3;
            $scope.max = 120;
            $scope.value = 10;
            
            $( 'body' ).append( $spinner );
            $scope.$digest();
        });
    });
    
    it( 'should have buttons by default', function(){
        
        var buttons = $spinner.find( '.button-parent' );
        expect( $(buttons).hasClass( 'ng-hide' ) ).toBe( false );
        expect( $(buttons).css( 'display' ) ).toBe( 'block' );
    });
    
    it( 'should go up and down by one as a default', function(){

        var increment = $spinner.find( '.increment' );
        
        increment.click();
    
        $scope.$apply();
        expect( $scope.value ).toBe( 11 );
        
        var decrement = $spinner.find( '.decrement' );
        decrement.click();
        decrement.click();
        $scope.$apply();
        expect( $scope.value ).toBe( 9 );
    });
    
    it( 'should handle a step different than the default', function(){
        $scope.step = 4;
        $scope.$apply();
        
        var inc = $spinner.find( '.increment' );
        var dec = $spinner.find( '.decrement' );
        
        inc.click();
        $scope.$apply();
        expect( $scope.value ).toBe( 14 );
        
        dec.click();
        dec.click();
        $scope.$apply();
        expect( $scope.value ).toBe( 6 );
    });
    
    it( 'should not allow the spinner to go beyond its bounds', function(){
        
        $scope.value = 2;
        $scope.$apply();
        
        expect( $scope.value ).toBe( 3 );
        
        $scope.value = 121;
        $scope.$apply();
        expect( $scope.value ).toBe( 120 );
        
        var inc = $spinner.find( '.increment' );
        inc.click();
        inc.click();
        $scope.$apply();
        expect( $scope.value ).toBe( 120 );
        
        $scope.value = 3;
        $scope.$apply();
        
        var dec = $spinner.find( '.decrement' );
        dec.click();
        dec.click();
        $scope.$apply();
        expect( $scope.value ).toBe( 3 );
        
        $scope.min = 10;
        $scope.$apply();
        expect( $scope.value ).toBe( 10 );
        
        $scope.value = 100;
        $scope.$apply();
        $scope.max = 90;
        $scope.$apply();
        expect( $scope.value ).toBe( 90 );
    });
    
    it( 'should increment with keyboard controls', function(){
        var input = $spinner.find( 'input' );
        input.trigger( {type: 'keydown', which: 38, keyCode: 38} );
        input.trigger( {type: 'keydown', which: 38, keyCode: 38} );
        
        $scope.$apply();
        expect( $scope.value ).toBe( 12 );
        
        $scope.value = 100;
        $scope.$apply();
        
        input.trigger( {type: 'keydown', which: 40, keyCode: 40} );
        input.trigger( {type: 'keydown', which: 40, keyCode: 40} );
        $scope.$apply();
        expect( $scope.value ).toBe( 98 );
    });
    
    it( 'should omit buttons when specified and key presses should still work', function(){
       
        inject( function( $compile, $rootScope ){
            
            var html = '<spinner value="value" min="min" max="max" step="step" show-buttons="false"></spinner>';
            $scope = $rootScope.$new();
            $spinner = angular.element( html );
            $compile( $spinner )( $scope );
            
            $scope.min = 3;
            $scope.max = 120;
            $scope.value = 10;
            
            $( 'body' ).append( $spinner );
            $scope.$digest();
        });
        
        var buttons = $spinner.find( '.button-parent' );
        expect( $(buttons).css( 'display' ) ).toBe( 'none' );
        expect( $(buttons).hasClass( 'ng-hide' ) ).toBe( true );
        
        var input = $spinner.find( 'input' );
        input.trigger( {type: 'keydown', which: 38, keyCode: 38} );
        input.trigger( {type: 'keydown', which: 38, keyCode: 38} );
        
        $scope.$apply();
        expect( $scope.value ).toBe( 12 );
        
        $scope.value = 100;
        $scope.$apply();
        
        input.trigger( {type: 'keydown', which: 40, keyCode: 40} );
        input.trigger( {type: 'keydown', which: 40, keyCode: 40} );
        $scope.$apply();
        expect( $scope.value ).toBe( 98 );
    });
});