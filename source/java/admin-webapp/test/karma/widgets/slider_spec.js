describe( 'Slider functionality', function(){
    
    var $slider, $scope;
    
    beforeEach( function(){
        
        module( function( $provide ){
            $provide.value( '$timeout', function( call, time ){
                setTimeout( call, time );
                return {};
            });
        });
        module( 'form-directives' );
        module( 'templates-main' );
        
        inject( function( $compile, $rootScope ){
            
            var html = '<div style="width: 1000px;"><slider min="0" max="10" step="1" ng-model="value" min-label="Minimum" max-label="Maximum"></slider></div>';
            $slider = angular.element( html );
            $scope = $rootScope.$new();
            $compile( $slider )( $scope );
            
            $scope.value = 5;
            
            $('body').append( $slider );
            $scope.$digest();      
            
        });
    });
    
    it( 'should should the widget correctly', function(){
        
        waitsFor( function(){
            $scope.$apply();
            var segments = $slider.find( '.segment' );
            
            if ( segments.length === 0 ){
                return false;
            }
            
            expect( segments.length ).toBe( 10 );
            expect( $slider.find( '.slider-handle' ).css( 'left' ) ).toBe( '495px' );
            expect( $slider.find( '.slider-labels .pull-left' ).text() ).toBe( 'Minimum' );
            expect( $slider.find( '.slider-labels .pull-right' ).text() ).toBe( 'Maximum' );
            expect( $($slider.find( '.segment-label' )[5]).hasClass( 'selected' ) ).toBe( true );
            expect( $($slider.find( '.segment-label' )[8]).hasClass( 'selected' ) ).toBe( false );
            
            return true;
        }, 500 );
    });
    
    it ( 'should redraw when the slider is moved correctly', function(){
        
        // not sure why this one doesn't need the waitFor call...
        
        $scope.$apply();
        var segments = $slider.find( '.segment' );

        $($slider.find( '.slider' )).trigger( {type: 'click', clientX: (100*8)} );

        $scope.$apply();
        expect( $scope.value ).toBe( 8 );
        expect( $slider.find( '.slider-handle' ).css( 'left' ) ).toBe( '795px' );
        expect( $($slider.find( '.segment-label' )[5]).hasClass( 'selected' ) ).toBe( false );
        expect( $($slider.find( '.segment-label' )[8]).hasClass( 'selected' ) ).toBe( true );

    });
    
    it( 'should redraw correctly when the bound value changes', function(){
        $scope.$apply();
        
        $scope.value = 2;
        
        waitsFor( function(){
            $scope.$apply();
            var segments = $slider.find( '.segment' );
            
            if ( segments.length === 0 ){
                return false;
            }

            expect( $scope.value ).toBe( 2 );
            expect( $slider.find( '.slider-handle' ).css( 'left' ) ).toBe( '195px' );
            expect( $($slider.find( '.segment-label' )[2]).hasClass( 'selected' ) ).toBe( true );
            expect( $($slider.find( '.segment-label' )[8]).hasClass( 'selected' ) ).toBe( false );
            
            return true;
        }, 500 );
    });
    
});