describe( 'Testing the modular tiering choice panels', function(){
    
    var $scope, $panel;
    
    beforeEach( function(){
        
        var fakeFilter;
        
        module( function( $provide ){
            
            $provide.value( 'translateFilter', fakeFilter );
        });
        
        fakeFilter = function( value ){
            return value;
        };
        
        module( 'volumes' );
        module( 'templates-main' );
        
        inject( function( $compile, $rootScope ){
            
            var html = '<tiering-panel ng-model="mediaPolicy"></tiering-panel>';
            
            $scope = $rootScope.$new();
            $panel = angular.element( html );
            $compile( $panel )( $scope );
            
            $scope.mediaPolicy = {};
            
            $( 'body' ).append( $panel );
            $scope.$digest();
        });
    });
    
    it( 'should set the media policy to ssd preferred', function(){

        $scope.mediaPolicy = 1;
        $scope.$apply();

        var buttons = $panel.find('.button');
        
        expect( $(buttons[1]).hasClass( 'selected' ) ).toBe( true );    
        expect( $scope.mediaPolicy.value ).toBe( 'SSD_PREFERRED' );
    });
    
    it( 'should set the media policy to SSD only', function(){
        
        $scope.mediaPolicy = {};
        $scope.$apply();

        var buttons = $panel.find('.button');
        
        expect( $(buttons[0]).hasClass( 'selected' ) ).toBe( true );
        expect( $scope.mediaPolicy.value ).toBe( 'SSD_ONLY' );
    });
    
    it( 'should set the media policy to HDD only', function(){
        
        $scope.mediaPolicy = 'HDD_ONLY';
        $scope.$apply();
        
        var buttons = $panel.find('.button');
        
        expect( $(buttons[2]).hasClass( 'selected' ) ).toBe( true );
        expect( $scope.mediaPolicy.value ).toBe( 'HDD_ONLY' );
    });
});