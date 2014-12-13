describe( 'Testing the button bar widget', function(){
    
    var $scope, $buttonbar;
    
    beforeEach( function(){
        
        module( 'form-directives' );
        module( 'templates-main' );
        
        inject( function( $compile, $rootScope ){
            var html = '<button-bar buttons="buttons" ng-model="selectedButton"></button-bar>';
            $scope = $rootScope.$new();
            $buttonbar = angular.element( html );
            $compile( $buttonbar )( $scope );
            
            $scope.buttons = [
                { label: 'Cow', value: 0 },
                { label: 'Chicken', value: 1 },
                { label: 'Koala', value: 2 },
                { label: 'Armadillo', value: 8 }
            ];
            
            $scope.selectedButton = $scope.buttons[0];
            $( 'body' ).append( $buttonbar );
            $scope.$digest();
        });
    });
    
    it( 'should draw a bar with 4 buttons', function(){
        
        var buttons = $buttonbar.find( '.button' );    
        expect( buttons.length ).toBe( 4 );
        
        // expect the first one to be selected and colored appropriately, the others should be white
        expect( $(buttons[0]).hasClass( 'selected' ) ).toBe( true );
        expect( $(buttons[0]).css( 'background-color' ) ).toBe( 'rgb(36, 134, 248)' );
        expect( $(buttons[1]).hasClass( 'selected' ) ).toBe( false );
        expect( $(buttons[1]).css( 'background-color' ) ).toBe( 'rgb(255, 255, 255)' );
        expect( $(buttons[2]).hasClass( 'selected' ) ).toBe( false );
        expect( $(buttons[2]).css( 'background-color' ) ).toBe( 'rgb(255, 255, 255)' );
        expect( $(buttons[3]).hasClass( 'selected' ) ).toBe( false );
        expect( $(buttons[3]).css( 'background-color' ) ).toBe( 'rgb(255, 255, 255)' );
        
    });
    
    it( 'should change the selection when another button is clicked', function(){
        
        var buttons = $buttonbar.find( '.button' );
        $(buttons[2]).click();
        
        $scope.$apply();
        
        // expect the third one to be selected and colored appropriately, the others should be white
        expect( $(buttons[2]).hasClass( 'selected' ) ).toBe( true );
        expect( $(buttons[2]).css( 'background-color' ) ).toBe( 'rgb(36, 134, 248)' );
        expect( $(buttons[0]).hasClass( 'selected' ) ).toBe( false );
        expect( $(buttons[0]).css( 'background-color' ) ).toBe( 'rgb(255, 255, 255)' );
        expect( $(buttons[1]).hasClass( 'selected' ) ).toBe( false );
        expect( $(buttons[1]).css( 'background-color' ) ).toBe( 'rgb(255, 255, 255)' );
        expect( $(buttons[3]).hasClass( 'selected' ) ).toBe( false );
        expect( $(buttons[3]).css( 'background-color' ) ).toBe( 'rgb(255, 255, 255)' );
        
        expect( $scope.selectedButton.value ).toBe( 2 );
    });
    
    it( 'should change the selection visually when the scope is set', function(){
        
        var buttons = $buttonbar.find( '.button' );
        $scope.selectedButton = $scope.buttons[3];
        $scope.$apply();
        
        // expect the fourth one to be selected and colored appropriately, the others should be white
        expect( $(buttons[3]).hasClass( 'selected' ) ).toBe( true );
        expect( $(buttons[3]).css( 'background-color' ) ).toBe( 'rgb(36, 134, 248)' );
        expect( $(buttons[0]).hasClass( 'selected' ) ).toBe( false );
        expect( $(buttons[0]).css( 'background-color' ) ).toBe( 'rgb(255, 255, 255)' );
        expect( $(buttons[1]).hasClass( 'selected' ) ).toBe( false );
        expect( $(buttons[1]).css( 'background-color' ) ).toBe( 'rgb(255, 255, 255)' );
        expect( $(buttons[2]).hasClass( 'selected' ) ).toBe( false );
        expect( $(buttons[2]).css( 'background-color' ) ).toBe( 'rgb(255, 255, 255)' );
        
        expect( $scope.selectedButton.value ).toBe( 8 );
    });
});