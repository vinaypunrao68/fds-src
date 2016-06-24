describe( 'Slider discrete list functionality', function(){
    
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
            
            var html = '<div style="width: 1000px;"><slider ng-model="value" min-label="Minimum" max-label="Maximum" values="values" label-function="getLabel"></slider></div>';
            $slider = angular.element( html );
            $scope = $rootScope.$new();
            $compile( $slider )( $scope );
            
             $scope.values = [
                 { value: 0, label: 'Cow' },
                 { value: 1, label: 'Chicken' },
                 { value: 2, label: 'Horse' },
                 { value: 3, label: 'Koala' },
                 { value: 4, label: 'Procupine' },
                 { value: 5, label: 'Liger' },
                 { value: 6, label: 'Elephant' },
                 { value: 7, label: 'Moose' },
                 { value: 8, label: 'David Hasselhoff' },
                 { value: 9, label: 'Turtle' }
             ];
             
             $scope.value = $scope.values[0];
             
             $scope.getLabel = function( item ){
                 return item.label;
             };
            
            $('body').append( $slider );
            $scope.$digest();      
            
        });
    });
    
    it( 'should handle a set of discreet values and label function', function(){
        waitsFor( function(){
            
            $scope.$apply();
            var segments = $slider.find( '.segment' );
            
            if ( segments.length === 0 ){
                return false;
            }
            
            expect( $scope.value.label ).toBe( 'Cow' );
            expect( $($slider.find( '.segment-label' )[0]).text() ).toBe( 'Cow' );
            return true;
            
        }, 500 );
    });
    
    it( 'should handle clicking the slider track with these values', function(){
        
        // not sure why this one doesn't need the waitFor call...
        
        $scope.$apply();
        var segments = $slider.find( '.segment' );

        $($slider.find( '.slider' )).trigger( {type: 'click', clientX: (100*9)} );

        $scope.$apply();
        expect( $scope.value.label ).toBe( 'David Hasselhoff' );
        expect( $slider.find( '.slider-handle' ).css( 'left' ) ).toBe( '891px' );
        expect( $($slider.find( '.segment-label' )[5]).hasClass( 'selected' ) ).toBe( false );
        expect( $($slider.find( '.segment-label' )[8]).hasClass( 'selected' ) ).toBe( true );
    });
    
    it( 'should redraw correctly when the bound value changes', function(){
        $scope.$apply();
        
        $scope.value = $scope.values[3];
        
        waitsFor( function(){
            $scope.$apply();
            var segments = $slider.find( '.segment' );
            
            if ( segments.length === 0 ){
                return false;
            }

            expect( $scope.value.label ).toBe( 'Koala' );
            expect( $slider.find( '.slider-handle' ).css( 'left' ) ).toBe( '331px' );
            expect( $($slider.find( '.segment-label' )[3]).hasClass( 'selected' ) ).toBe( true );
            expect( $($slider.find( '.segment-label' )[8]).hasClass( 'selected' ) ).toBe( false );
            
            return true;
        }, 500 );
    });
});