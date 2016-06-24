describe( 'Radio button functionality', function(){
    
    var $scope, $radio1, $radio2;
    
    beforeEach( function(){
        
        module( 'form-directives' );
        module( 'templates-main' );
        
        inject( function( $compile, $rootScope ){
            
            var html = '<div><fds-radio class="radio1" name="testbox" ng-model="value" value="car" label="Car" enabled="oneEnabled"></fds-radio>'+
                '<fds-radio class="radio2" name="testbox" ng-model="value" value="boat" label="Boat" enabled="twoEnabled"></fds-radio></div>'
            $scope = $rootScope.$new();
            var $radio = angular.element( html );
            $compile( $radio )( $scope );
            
            $scope.value = 'car';
            $scope.oneEnabled = true;
            $scope.twoEnabled = true;
            
            $('body').append( $radio );
            $scope.$digest();
            
            $radio1 = $radio.find( '.radio1' );
            $radio2 = $radio.find( '.radio2' );
        });
    });
    
    it( 'should select the first radio by default', function(){
       
        expect( $radio1.find( '.inner-ring' ).css( 'display' ) ).toBe( 'block' );
        expect( $radio2.find( '.inner-ring' ).css( 'display' ) ).toBe( 'none' );
        expect( $radio2.find( '.inner-ring' ).hasClass( 'ng-hide' ) ).toBe( true );
        
        expect( $radio1.find( 'label' ).text() ).toBe( 'Car' );
        expect( $radio2.find( 'label' ).text() ).toBe( 'Boat' );
    });
    
    it( 'should change the selection based on the scope value', function(){
        
        $scope.value = 'boat';
        $scope.$apply();
        
        expect( $radio2.find( '.inner-ring' ).css( 'display' ) ).toBe( 'block' );
        expect( $radio1.find( '.inner-ring' ).css( 'display' ) ).toBe( 'none' );
        expect( $radio1.find( '.inner-ring' ).hasClass( 'ng-hide' ) ).toBe( true );
    });
    
    it( 'should change the value when clicked', function(){
        
        $radio2.find( '.selector' ).click();
        
        expect( $radio2.find( '.inner-ring' ).css( 'display' ) ).toBe( 'block' );
        expect( $radio1.find( '.inner-ring' ).css( 'display' ) ).toBe( 'none' );
        expect( $radio1.find( '.inner-ring' ).hasClass( 'ng-hide' ) ).toBe( true );
        expect( $scope.value ).toBe( 'boat' );
        
        $radio1.find( '.selector' ).click();
        
        expect( $radio1.find( '.inner-ring' ).css( 'display' ) ).toBe( 'block' );
        expect( $radio2.find( '.inner-ring' ).css( 'display' ) ).toBe( 'none' );
        expect( $radio2.find( '.inner-ring' ).hasClass( 'ng-hide' ) ).toBe( true );
        expect( $scope.value ).toBe( 'car' );        
    });
    
    it( 'should not allow you to click on a disabled box', function(){
        
        $scope.twoEnabled = false;
        $scope.$apply();
        
        expect( parseFloat( $radio2.find( '.selector' ).css( 'opacity' ) ).toFixed( 1 ) ).toBe( '0.4' );
        
        $radio2.find( '.selector' ).click();
        
        expect( $scope.value ).toBe( 'car' );
        expect( $radio1.find( '.inner-ring' ).css( 'display' ) ).toBe( 'block' );
        expect( $radio2.find( '.inner-ring' ).css( 'display' ) ).toBe( 'none' );
    });
});